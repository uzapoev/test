#ifndef __SpirvAnalyzer_h__
#define __SpirvAnalyzer_h__

#include <assert.h>
#include <vulkan/spirv.h>

#include <string>
#include <vector>
#include <map>


class SpirvAnalyzer
{ 
    enum Decoration {
        DecorationBuiltIn = 11,
        DecorationLocation = 30,
        DecorationBinding = 33,
        DecorationDescriptorSet = 34,
    };
    enum StorageClass {
        StorageClassUniformConstant = 0,
        StorageClassInput = 1,
        StorageClassUniform = 2,
        StorageClassOutput = 3,
        StorageClassWorkgroup = 4,
        StorageClassCrossWorkgroup = 5,
        StorageClassPrivate = 6,
        StorageClassFunction = 7,
        StorageClassGeneric = 8,
        StorageClassPushConstant = 9,
        StorageClassAtomicCounter = 10,
        StorageClassImage = 11,
        StorageClassStorageBuffer = 12,
        StorageClassMax = 0x7fffffff,
    };

    enum UniformType
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


#define RETURNCASE( value ) case value: return #value;
    static const char * storage2str(StorageClass s)
    {
        switch (s)
        {
            RETURNCASE(StorageClassUniformConstant)
            RETURNCASE(StorageClassInput          )
            RETURNCASE(StorageClassUniform        )
            RETURNCASE(StorageClassOutput         )
            RETURNCASE(StorageClassWorkgroup      )
            RETURNCASE(StorageClassCrossWorkgroup )
            RETURNCASE(StorageClassPrivate        )
            RETURNCASE(StorageClassFunction       )
            RETURNCASE(StorageClassGeneric        )
            RETURNCASE(StorageClassPushConstant   )
            RETURNCASE(StorageClassAtomicCounter  )
            RETURNCASE(StorageClassImage          )
            RETURNCASE(StorageClassStorageBuffer  )
        }
        return "unknown storage";
    }

    static const char * type2str(SpvOp op)
    {
        switch (op)
        {
            RETURNCASE( SpvOpTypeVoid   )   RETURNCASE( SpvOpTypeBool )         RETURNCASE( SpvOpTypeInt )
            RETURNCASE( SpvOpTypeFloat  )   RETURNCASE( SpvOpTypeVector )       RETURNCASE( SpvOpTypeMatrix )
            RETURNCASE( SpvOpTypeImage  )   RETURNCASE( SpvOpTypeSampler )      RETURNCASE( SpvOpTypeSampledImage )
            RETURNCASE( SpvOpTypeArray  )   RETURNCASE( SpvOpTypeRuntimeArray ) RETURNCASE( SpvOpTypeStruct )
            RETURNCASE( SpvOpTypeOpaque )   RETURNCASE( SpvOpTypePointer )      RETURNCASE( SpvOpTypeFunction )
            RETURNCASE( SpvOpTypeEvent  )   RETURNCASE( SpvOpTypeDeviceEvent )  RETURNCASE( SpvOpTypeReserveId )
            RETURNCASE( SpvOpTypeQueue  )   RETURNCASE( SpvOpTypePipe )         RETURNCASE( SpvOpTypeForwardPointer )
        }
        return "unknown type";
    }
#undef RETURNCASE

    static const int SPIRV_MAGIC = 0x07230203;
    static const int SPIRV_MAGIC_REV = 0x03022307;
    static const int SPIRV_VERSION = 0x00010000;

    struct SpirvHeader
    {
        uint32_t            magic;
        uint32_t            version;
        uint32_t            generator;
        uint32_t            bound;
        uint32_t            reserved;
    };

    struct InstructionHeader
    {
        uint16_t               op;    //uint16_t opcode : 16; //
        uint16_t            count; //uint16_t wordCount : 16;
    };

    struct Instruction
    {
        Instruction(InstructionHeader * _header, uint32_t _sise, uint32_t _offset)
        :header(_header), size(_sise), offset(_offset){}

        InstructionHeader * header;
        uint32_t            size;
        uint32_t            offset;
    };

    struct stream_view
    {
        stream_view(const char * d, size_t size) :m_data(d), m_currptr(d), m_size(size){};

        template <class T> const T *    shift() const                       { const T * ret = (const T*)(m_currptr); m_currptr += sizeof(T); return ret; }
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

    struct string_view
    {
        string_view(const char * d) :m_data(d)                              { if (m_data) m_size = strlen(m_data); }
        string_view(const char * d, size_t size) :m_data(d), m_size(size)   {}
        string_view(const string_view & sview) 
            :m_data(sview.data()), m_size(sview.size())                     {}

        size_t                          size() const                        { return m_size; }
        size_t                          lenght() const                      { return m_size; }

        const char *                    data() const                        { return m_data; }
        const char *                    c_str() const                       { return m_data; }

        operator const char* ()                                             { return m_data; }
        operator const char* () const                                       { return m_data; }

    private:
        mutable const char *            m_data;
        size_t                          m_size;
    };

    class Decorate
    {
    public:
        Decorate(uint32_t _id, const string_view & _name) :id(_id), m_name(_name){}//, description(0), binding(0){}

        void add(Decorate & decorate)   { m_childs.push_back(decorate); }

        Decorate * child(uint32_t id) {
            for (size_t i = 0; i < m_childs.size(); i++)
                if (m_childs[i].id == id)
                    return &m_childs[i];
            return nullptr;
        }

        const string_view &             name() const { return m_name; }

    private:
        string_view                     m_name;
        uint32_t                        id = UINT32_MAX;

        std::vector<Decorate>               m_childs;
        std::map<SpvDecoration, uint32_t>   m_decors;
    };


    static bool istype(SpvOp op)
    {
        return op >= SpvOpTypeVoid && op <= SpvOpTypePipe;
    }

    struct Constant
    {
        SpvOp type;
        union
        {
            int i;
            float f;
        }value;
    };

    struct TypeInfo
    {
        SpvOp       spvtype = SpvOpNop;
        UniformType type = unusable;
        uint32_t    count = 0;
        uint32_t    basetypeid = 0;
    };


public:
    struct Uniform
    {
        const char *    name;
        uint32_t        offset;
        UniformType     type;
    };

    struct UniformBufferObject
    {
        const char *    name;
        size_t          size;
        size_t          count;
        Uniform *       uniforms;
    };

    struct Attribute
    {
        char * name;
        int location;
    };

    struct SpvInfo
    {
        size_t                  acount;
        Attribute *             attributes;

        size_t                  ucount;
        UniformBufferObject *   uniforms;
    };

    // SpirvAnalyzer.analyze(data, size, &ubo, &pushcostants, &samplers);
    static bool analyze(const void * data, size_t size, std::vector<Uniform> * uniforms, UniformBufferObject ** ubos = NULL)
    {
        stream_view stream((const char *)data, size);

        const SpirvHeader * h = stream.shift<SpirvHeader>();
        if (h->magic != SPIRV_MAGIC || h->version != SPIRV_VERSION)
            return false;

        SpirvAnalyzer * analizer = new SpirvAnalyzer();
        std::vector<Instruction> instructions;
        while (!stream.eof())
        {
            InstructionHeader * instr = (InstructionHeader*)stream.shift<InstructionHeader>();
            instructions.emplace_back(instr, instr->count - 1, stream.tell());

            size_t seekpos = stream.tell() + (instr->count - 1) * sizeof(uint32_t);
            stream.seek(seekpos);
        }

        for (size_t i = 0; i < instructions.size(); i++)
        {
            char buff[256] = "";

            auto instruction = instructions[i];
            stream.seek(instruction.offset);

            SpvOp op = (SpvOp)instruction.header->op;

            if (istype(op))
            {
                uint32_t id = stream.read<uint32_t>();
                TypeInfo ti = parse_type(id, stream, instruction, analizer);

                analizer->m_types.emplace(id, ti);
                continue;
            }

            switch (op)
            {
                case SpvOpName: //5: // name
                {
                    uint32_t id = stream.read<uint32_t>();
                    const char * name = stream.map();
                    analizer->m_decorates.emplace(id, Decorate(id, string_view(name)));
                } break;

                case SpvOpMemberName: //6: //member name
                {
                    uint32_t parentid = stream.read<uint32_t>();
                    uint32_t memberid = stream.read<uint32_t>();
                    const char * name = stream.map();

                    auto it = analizer->m_decorates.find(parentid);
                    it->second.add(Decorate(memberid, string_view(name)));
                }break;

                case SpvOpVariable: // 59://variable 
                {
                    uint32_t vartype     = stream.read<uint32_t>();
                    uint32_t decorid     = stream.read<uint32_t>();
                    StorageClass storage = stream.read<StorageClass>();

                    auto decor = analizer->get_decor(decorid);
                    auto type = analizer->get_type(vartype);

                    if (storage == StorageClass::StorageClassUniform || storage == StorageClass::StorageClassUniformConstant)
                    {
                        if (uniforms)
                        {
                            Uniform uniform = {};
                            uniform.name = decor ? decor->name() : "";
                            if (type.type == UniformType::structure)
                            {

                            }
                           // strcpy(uniform.name, decor?decor->name():"");
                            uniforms->push_back(uniform);
                        }
                    }

                    // attributes
                    if (storage == StorageClass::StorageClassInput)
                    {

                    }
                }break;

                case SpvOpConstant:
                {
                    uint32_t type = stream.read<uint32_t>();
                    uint32_t id = stream.read<uint32_t>();
                    uint32_t value = stream.read<uint32_t>();
                    Constant c = { analizer->m_types[type].spvtype, value };
                    analizer->m_constants.emplace(id, c);
                    assert(instruction.size == 3);
                }break;
            }
        }
        printf("\n");
        return true;
    }

private:
    static TypeInfo parse_type(uint32_t id, stream_view & stream, Instruction & instruction, SpirvAnalyzer *analizer)
    {
        TypeInfo typeInfo;
        SpvOp spvtype = (SpvOp)instruction.header->op;
        printf("\ntype %-18s id(%2d) ", type2str(spvtype), id);
        size_t filedcount = instruction.size - 1;

        typeInfo.spvtype = spvtype;

        switch (spvtype)
        {
            case SpvOpTypeInt:
            case SpvOpTypeFloat: {
                uint32_t bytecount = stream.read<uint32_t>() / 8;

                typeInfo.type = (spvtype == SpvOpTypeInt) ? (UniformType)(UniformType::int8 + (bytecount / 2)) :
                                                            (UniformType)(UniformType::float16 + (bytecount / 2) - 1);
            } break;

            case SpvOpTypeVector:{
                uint32_t basetypeid = stream.read<uint32_t>();
                uint32_t count = stream.read<uint32_t>();

                auto basetype = analizer->get_type(basetypeid);
                if (basetype.type >= UniformType::int8 && basetype.type <= UniformType::int64){
                    typeInfo.type = (UniformType)(UniformType::vec2i + (count / 2));
                }
                if (basetype.type >= UniformType::float16 && basetype.type <= UniformType::float32){
                    typeInfo.type = (UniformType)(UniformType::vec2f + (count / 2));
                }
            }break;

            case SpvOpTypeMatrix:{
                uint32_t basetypeid = stream.read<uint32_t>();
                uint32_t count = stream.read<uint32_t>();

                auto basetype = analizer->get_type(basetypeid);
                if (basetype.type >= UniformType::vec3f && basetype.type <= UniformType::vec4f){
                    typeInfo.type = (UniformType)(UniformType::mat3 + (count / 2) - 1);
                }
            }break;

            case SpvOpTypeArray:{
                uint32_t basetypeid = stream.read<uint32_t>();
                uint32_t constantid = stream.read<uint32_t>();

                typeInfo = analizer->get_type(basetypeid);
                typeInfo.count = analizer->m_constants[constantid].value.i;
            }break;

            case SpvOpTypeStruct:{
                UniformBufferObject * ubo = new UniformBufferObject();
                auto decor = analizer->get_decor(id);
                ubo->name = decor->name();
                ubo->count = filedcount;
                ubo->uniforms = new Uniform[filedcount]();

                analizer->m_ubos.insert(std::make_pair(id, ubo));
                typeInfo.type = UniformType::structure;

                for (size_t u = 0; u < filedcount; ++u){
                    Decorate * fieldinfo = decor->child(u);

                    uint32_t idtype = stream.read<uint32_t>();
                    auto type = analizer->get_type(idtype);
                    ubo->uniforms[u].name = fieldinfo->name();
                    printf("%s(%d) ", type2str(analizer->get_type(idtype).spvtype), idtype);
                }
            }break;

            case SpvOpTypePointer:{
                SpvStorageClass storage = (SpvStorageClass)stream.read<uint32_t>();
                uint32_t basetypeid = stream.read<uint32_t>();

                auto type = analizer->get_type(basetypeid);

                typeInfo.type = type.type;
                typeInfo.basetypeid = basetypeid;

                printf("\ntype %-18s id(%2d) ", type2str(spvtype), id);
            }break;

            default:{
                for (size_t j = 0; j < filedcount; ++j)
                    printf("%-3d", stream.read<uint32_t>());
            }break;
        }
        return typeInfo;
    }

    Decorate *  get_decor(uint32_t id)          { auto it = m_decorates.find(id); return (it == m_decorates.end()) ? nullptr : &it->second; }
    TypeInfo    get_type(int id)                { return m_types[id]; }
    Constant    get_const(int id)               { return m_constants[id]; }

protected:
    std::map<uint32_t, Decorate>                m_decorates;
    std::vector<Instruction>                    m_instructions;
    std::map<int, TypeInfo>                     m_types;      // type / parent
    std::map<int, Constant>                     m_constants;  // { id, {type, value} }

    std::map<uint32_t, UniformBufferObject*>    m_ubos;  // { id, {type, value} }
};


#endif