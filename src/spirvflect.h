#ifndef __Spirvflect_h__
#define __Spirvflect_h__

#include <assert.h>
#include <vulkan/spirv.h>

#include <string>
#include <vector>
#include <map>
#include <unordered_map>


class Spirvflect
{
public:

    enum UniformType : uint16_t
    {
        unusable,
        int8, int16, int32, int64,
        float16, float32, float64,
        vec2i, vec3i, vec4i,
        vec2f, vec3f, vec4f,
        mat3, mat4,
        structure,
        sampler2d, sampler3d, samplerCube,
    };

    struct Uniform
    {
        const char *    name;   // 32
        uint16_t        offset; // 16
        UniformType     type;   // 16
    };

    struct UniformBufferObject
    {
        const char *    name;
        size_t          size;
        size_t          count;
        Uniform         uniforms[128];
    };

    struct Attribute
    {
        char * name;
        int location;
    };

private:
    struct stream_view
    {
        stream_view(const char * d, size_t size) :m_data(d), m_currptr(d), m_size(size){};
        template <class T> T            read()                              { T ret; read(&ret, sizeof(T)); return ret;  }

        void                            read(void * data, uint32_t size)    { memcpy(data, m_currptr, size); m_currptr += size; }
        const char *                    data() const                        { return m_data; }
        const char *                    map() const                         { return m_currptr; }

        void                            seek(size_t pos)                    { m_currptr = m_data + pos; }
        size_t                          tell() const                        { return m_currptr - m_data; }
        bool                            eof() const                         { return tell() >= m_size; }

    private:
        mutable const char *            m_data;
        mutable const char *            m_currptr;
        size_t                          m_size;
    };

private:
    static const int SPIRV_MAGIC = 0x07230203;
    static const int SPIRV_MAGIC_REV = 0x03022307;
    static const int SPIRV_VERSION = 0x00010000;

    struct SpvHeader
    {
        uint32_t            magic;
        uint32_t            version;
        uint32_t            generator;
        uint32_t            bound;
        uint32_t            reserved;
    };

    struct InstructionHeader
    {
        uint16_t            opcode; // uint16_t opcode : 16;
        uint16_t            count;  // uint16_t wordCount : 16;
    };

    struct SpvConstant
    {
        UniformType type;
        union
        {
            int i;
            float f;
        }value;
    };

    struct SpvTypeInfo
    {
        UniformType type = unusable;
        uint32_t    count = 0;
        uint32_t    basetypeid = 0;
    };

    class SpvDecorate
    {
    public:
        SpvDecorate(uint32_t _id, const char * _name) :id(_id), m_name(_name)        {}//, description(0), binding(0){}

        void                            add_child(SpvDecorate & decorate)                   { m_childs.push_back(decorate); }

        void                            set_decor(SpvDecoration decor, uint32_t value)      { m_decors[decor] = value; }
        bool                            has_decor(SpvDecoration decor)                      { return m_decors.count(decor) > 0; }
        uint32_t                        get_decor(SpvDecoration decor)                      { return m_decors[decor]; }

        const char                    * name() const                                        { return m_name; }

        SpvDecorate                   * get_child(uint32_t id) {
            for (size_t i = 0; i < m_childs.size(); i++)
            if (m_childs[i].id == id)
                return &m_childs[i];
            return nullptr;
        }

    private:
        const char                        * m_name;
        uint32_t                            id = UINT32_MAX;

        std::vector<SpvDecorate>            m_childs;
        std::map<SpvDecoration, uint32_t>   m_decors;
      //  std::unordered_map<uint32_t, SpvDecorate> m_chhh;
    };

    class SpvStorage
    {
    public:
        void                    add_decor(uint32_t id, SpvDecorate & d)     { m_decorates.emplace(id, d); }
        SpvDecorate           * get_decor(uint32_t id)                      { auto it = m_decorates.find(id); return (it == m_decorates.end()) ? nullptr : &it->second; }

        void                    add_type(uint32_t id, SpvTypeInfo ti)       { m_types.emplace(id, ti); };
        SpvTypeInfo             get_type(uint32_t id)                       { return m_types[id]; }

        void                    add_const(uint32_t id, SpvConstant &c)      { m_constants.emplace(id, c); }
        SpvConstant             get_const(uint32_t id)                      { return m_constants[id]; }

        void                    add_ubo(uint32_t id, UniformBufferObject*u) { m_ubos.emplace(id, u); }
        UniformBufferObject   * get_ubo(uint32_t id)                        { auto it = m_ubos.find(id); return (it == m_ubos.end()) ? nullptr : it->second; }

    private:
        std::map<uint32_t, SpvDecorate>                     m_decorates;
        std::map<uint32_t, SpvTypeInfo>              m_types;      // type / parent
        std::map<uint32_t, SpvConstant>              m_constants;  // { id, {type, value} }
        std::map<uint32_t, UniformBufferObject*>     m_ubos;  // { id, {type, value} }
    };

public:
    // SpirvAnalyzer.analyze(data, size, &ubo, &pushcostants, &samplers);
    static bool analyze(const void * data, size_t size, std::vector<UniformBufferObject*> * uniforms, std::vector<Uniform> * smaplers)
    {
        stream_view stream((const char *)data, size);

        SpvHeader  header = stream.read<SpvHeader>();
        if (header.magic != SPIRV_MAGIC || header.version != SPIRV_VERSION)
            return false;

        SpvStorage storage;
        while (!stream.eof())
        {
            InstructionHeader instruction = stream.read<InstructionHeader>();
            size_t next = stream.tell() + sizeof(uint32_t)* (instruction.count-1);

            SpvOp opcode = (SpvOp)instruction.opcode;
            switch (opcode)
            {
                case SpvOpTypeBool:
                case SpvOpTypeInt:
                case SpvOpTypeFloat:
                case SpvOpTypeVector:
                case SpvOpTypeMatrix:
                case SpvOpTypeImage:
                case SpvOpTypeSampler:
                case SpvOpTypeSampledImage:
                case SpvOpTypeArray:
                case SpvOpTypeRuntimeArray:
                case SpvOpTypeStruct:
                case SpvOpTypePointer:
                {
                    uint32_t id = stream.read<uint32_t>();
                    SpvTypeInfo ti = parse_type(id, stream, instruction, &storage);
                    storage.add_type(id, ti);
                } break;

                case SpvOpConstant:
                {
                    uint32_t type = stream.read<uint32_t>();
                    uint32_t id = stream.read<uint32_t>();
                    uint32_t value = stream.read<uint32_t>();
                    SpvConstant c = { storage.get_type(type).type, value };
                    storage.add_const(id, c);
                }break;


                case SpvOpName: //5: // name
                {
                    uint32_t id = stream.read<uint32_t>();
                    const char * name = stream.map();
                    storage.add_decor(id, SpvDecorate(id, name));
                } break;


                case SpvOpMemberName: //6: //member name
                {
                    uint32_t parentid = stream.read<uint32_t>();
                    uint32_t memberid = stream.read<uint32_t>();
                    const char * name = stream.map();

                    auto decor = storage.get_decor(parentid);
                    if (decor)
                        decor->add_child(SpvDecorate(memberid, name));
                }break;


                case SpvOpDecorate:
                {
                    uint32_t id = stream.read<uint32_t>();
                    SpvDecoration d = stream.read<SpvDecoration>();
                    
                    auto decor = storage.get_decor(id);
                    if (decor /*&& d == SpvDecoration::SpvDecorationLocation*/)
                    {
                        uint32_t value = stream.read<uint32_t>();
                        decor->set_decor(d, value);
                    }
                } break;


                case SpvOpMemberDecorate:
                {
                     uint32_t parentId = stream.read<uint32_t>();
                     uint32_t memberId = stream.read<uint32_t>();
                     SpvDecoration d = stream.read<SpvDecoration>();
                     uint32_t value = stream.read<uint32_t>();

                     SpvDecorate * parentDecor = storage.get_decor(parentId);
                     SpvDecorate * memberDdecor = parentDecor->get_child(memberId);
                     memberDdecor->set_decor(d, value);
                }break;


                case SpvOpVariable: // 59://variable 
                {
                    uint32_t vartype = stream.read<uint32_t>();
                    uint32_t decorid = stream.read<uint32_t>();
                    SpvStorageClass storageClass = stream.read<SpvStorageClass>();

                    SpvDecorate * decor = storage.get_decor(decorid);
                    SpvTypeInfo  type = storage.get_type(vartype);

                    switch (storageClass)
                    {
                        case SpvStorageClass::SpvStorageClassUniform:
                        {
                            if (uniforms && type.type == UniformType::structure)
                            {
                                auto ubo = storage.get_ubo(type.basetypeid);
                                auto dec = storage.get_decor(type.basetypeid);

                                uniforms->push_back(ubo);
                            }
                        }break;

                        // attributes
                        case SpvStorageClass::SpvStorageClassInput:
                        {
                            if (decor->has_decor(SpvDecoration::SpvDecorationLocation))
                            {
                                uint32_t location = decor->get_decor(SpvDecoration::SpvDecorationLocation);
                            }
                        }break;
                    }       
                }break;
            }
            stream.seek(next);
        }

        printf("\n");
        return true;
    }

private:
    static SpvTypeInfo parse_type(uint32_t id, stream_view & stream, InstructionHeader & instruction, SpvStorage *storage)
    {
        SpvTypeInfo typeInfo = {};
        size_t filedcount = instruction.count - 2;

        switch (instruction.opcode)
        {
            case SpvOpTypeInt:
            {
                uint32_t bytecount = stream.read<uint32_t>() / 8;

                typeInfo.type = (UniformType)(UniformType::int8 + (bytecount / 2));
            }break;

            case SpvOpTypeFloat:
            {
                uint32_t bytecount = stream.read<uint32_t>() / 8;

                typeInfo.type = (UniformType)(UniformType::float16 + (bytecount / 2) - 1);
            } break;

            case SpvOpTypeVector:
            {
                uint32_t basetypeid = stream.read<uint32_t>();
                uint32_t count = stream.read<uint32_t>();

                auto basetype = storage->get_type(basetypeid);
                if (basetype.type >= UniformType::int8 && basetype.type <= UniformType::int64){
                    typeInfo.type = (UniformType)(UniformType::vec2i + (count / 2));
                }
                if (basetype.type >= UniformType::float16 && basetype.type <= UniformType::float32){
                    typeInfo.type = (UniformType)(UniformType::vec2f + (count / 2));
                }
            }break;

            case SpvOpTypeMatrix:
            {
                uint32_t basetypeid = stream.read<uint32_t>();
                uint32_t count = stream.read<uint32_t>();

                auto basetype = storage->get_type(basetypeid);
                if (basetype.type >= UniformType::vec3f && basetype.type <= UniformType::vec4f){
                    typeInfo.type = (UniformType)(UniformType::mat3 + (count / 2) - 1);
                }
            }break;

            case SpvOpTypeArray:
            {
                uint32_t basetypeid = stream.read<uint32_t>();
                uint32_t constantid = stream.read<uint32_t>();

                typeInfo = storage->get_type(basetypeid);
                typeInfo.count = storage->get_const(constantid).value.i;
            }break;

            case SpvOpTypeStruct:
            {
                typeInfo.type = UniformType::structure;
                auto decor = storage->get_decor(id);

                UniformBufferObject * ubo = new UniformBufferObject();

                ubo->name = decor->name();
                ubo->count = filedcount;

                for (size_t u = 0; u < filedcount; ++u)
                {
                    SpvDecorate * fieldDecor = decor->get_child(u);
                    uint32_t type_id = stream.read<uint32_t>();

                    auto type = storage->get_type(type_id);
                    ubo->uniforms[u].name = fieldDecor->name();
                    ubo->uniforms[u].type = type.type;
                }
                storage->add_ubo(id, ubo);
            }break;

            case SpvOpTypePointer:
            {
                SpvStorageClass storagetype = (SpvStorageClass)stream.read<uint32_t>();
                uint32_t typeId = stream.read<uint32_t>();

                typeInfo = storage->get_type(typeId);
                typeInfo.basetypeid = typeId;
            }break;

            case SpvOpTypeImage:
            case SpvOpTypeSampler:
            case SpvOpTypeSampledImage:
            {
               printf("\n");
                for (size_t j = 0; j < filedcount; ++j)
                    printf("%-3d", stream.read<uint32_t>());
            }break;

            default:
            {
                assert(false);
                for (size_t j = 0; j < filedcount; ++j)
                    printf("%-3d", stream.read<uint32_t>());
            }break;
        }
        return typeInfo;
    }
};


#endif