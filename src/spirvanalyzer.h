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

        void                            seek(size_t pos)                    { m_currptr = m_data + pos; }
        size_t                          tell() const                        { return m_currptr - m_data; }
        bool                            eof() const                         { return tell() >= m_size; }

    private:
        mutable const char *            m_data;
        mutable const char *            m_currptr;
        size_t                          m_size;
    };

    class Decorate
    {
    public:
        Decorate(uint32_t _id, const std::string & _name) :id(_id), name(_name){}//, description(0), binding(0){}

        void add(Decorate & decorate)   { m_childs.push_back(decorate); }

        Decorate * child(uint32_t id) {
            for (size_t i = 0; i < m_childs.size(); i++)
                if (m_childs[i].id == id)
                    return &m_childs[i];
            return nullptr;
        }

        std::string                     name;
        uint32_t                        id = UINT32_MAX;

        std::vector<Decorate>               m_childs;
        std::map<SpvDecoration, uint32_t>   m_decors;
    };


    static bool istype(SpvOp op)
    {
        return op >= SpvOpTypeVoid && op <= SpvOpTypePipe;
    }

    struct constant{
        int type;
        union{
            int i;
            float f;
        }value;
    };

public:
    /*
        Uniform {
            name,
            buffer_id,
            offset,
        }
    
    */
    struct Uniform
    {
        char name[256];
        int location;
        SpvOp type;
        int count;
    };
    class UniformBufferObject
    {
        Uniform uniforms[512];
    };

    class Attribute
    {
    };

    static bool analyze(const void * data, size_t size, std::vector<Uniform> * uniforms)
    {
        stream_view stream((const char *)data, size);

        const SpirvHeader * h = stream.shift<SpirvHeader>();
        if (h->magic != SPIRV_MAGIC || h->version != SPIRV_VERSION)
            return false;

        std::map<uint32_t, Decorate>    decorates;
        std::vector<Instruction>        instructions;
        std::map<SpvOp, int>            types;      // type / parent
        std::map<int, constant>         constants;  // { id, {type, value} }
         
        std::vector<SpvOp>ops;
        while (!stream.eof())
        {
            InstructionHeader * instr = (InstructionHeader*)stream.shift<InstructionHeader>();
            instructions.emplace_back(instr, instr->count - 1, stream.tell());

            ops.push_back(SpvOp(instr->op));

            for (uint16_t i = 0; i < instr->count - 1; i++)
            {
                stream.shift<uint32_t>();
            }
        }
        for (size_t i = 0; i < instructions.size(); i++)
        {
            char buff[64] = "";

            auto instruction = instructions[i];
            stream.seek(instruction.offset);

            SpvOp op = (SpvOp)instruction.header->op;
            if (op == SpvOpConstant){

            }
            else if (istype(op))
            {
                printf("\ntype %s", type2str(op));
                parse_type(instruction, stream);
                continue;
            }

            switch (op)
            {
                case SpvOpName: //5: // name
                {
                    uint32_t id = stream.read<uint32_t>();
                    stream.read(buff, sizeof(uint32_t)* instruction.size - 1);

                    assert(decorates.find(id) == decorates.end());
                    decorates.insert(std::make_pair(id, Decorate(id, buff)));
                } break;

                case SpvOpMemberName: //6: //member name
                {
                    uint32_t id     = stream.read<uint32_t>();
                    uint32_t mid    = stream.read<uint32_t>();

                    stream.read(buff, sizeof(uint32_t)* instruction.size - 2);

                    auto it = decorates.find(id);
                    it->second.add(Decorate(mid, buff));
                }break;

                case SpvOpDecorate: // 71:// decorate
                {
                    uint32_t id         = stream.read<uint32_t>();
                    SpvDecoration decor = stream.read<SpvDecoration>();

                    auto it = decorates.find(id);
                    const char * membername = it != decorates.end() ? it->second.name.c_str() : "";
                    for (size_t j = 0; j < instruction.size - 2; ++j)
                    {
                        uint32_t data = 0;
                        stream.read(&data, sizeof(uint32_t));

                        if (it != decorates.end())
                            it->second.m_decors.emplace(decor, data);

                        printf("\ndecorate: %d(%s) %d %d", id, membername, decor, data);
                    }
                }break;

                case SpvOpMemberDecorate: // 72: // member decorate
                {
                    uint32_t id         = stream.read<uint32_t>();
                    uint32_t mid        = stream.read<uint32_t>();
                    SpvDecoration decor = stream.read<SpvDecoration>();
  
                    auto it = decorates.find(id);
                    if (it == decorates.end())
                        break;

                    auto child = it->second.child(mid);
                    const char * membername = child ? child->name.c_str() : "";
                    printf("\nmember decorate: %d(%s) %d(%s) %d", id, it->second.name.c_str(), mid, membername, decor);

                    for (size_t j = 0; j < instruction.size - 3; ++j){
                        uint32_t data = stream.read<uint32_t>();
                        if (child)
                            child->m_decors.emplace(decor, data);

                        printf(" %d", data);
                    }
                }break;

                case SpvOpVariable: // 59://variable 
                {
                    uint32_t type       = stream.read<uint32_t>();
                    uint32_t id         = stream.read<uint32_t>();
                    StorageClass storage= stream.read<StorageClass>();

                    if (storage == StorageClass::StorageClassUniform || storage == StorageClass::StorageClassUniformConstant)
                    {
                        if (uniforms)
                        {
                            uniforms->push_back(Uniform());
                            auto decor = decorates.find(id);
                            if (decor != decorates.end())
                                strcpy(uniforms->back().name, decor->second.name.c_str());
                            
                        }
                    }

                    // attributes
                    if (storage == StorageClass::StorageClassInput)
                    {

                    }

                    auto it = decorates.find(id);
                    if (it != decorates.end())
                        printf("\nvariable: %d(%s) %d %s", type, it->second.name.c_str(), id, storage2str(storage));
                    else
                        printf("\nvariable: %d %d %s", type, id, storage2str(storage));
                }break;

                case SpvOpConstant:
                {
                    uint32_t type = stream.read<uint32_t>();
                    uint32_t id = stream.read<uint32_t>();
                    uint32_t value = stream.read<uint32_t>();
                    assert(instruction.size == 3);
                    printf("\n(%d) constant: id(%d)  value(%d)", type, id, value);
                   /* for (size_t j = 0; j < instruction.size - 1; ++j){
                        uint32_t value = stream.read<uint32_t>();
                        printf(" %d", value);
                    }*/
                }break;
            }
        }
        printf("\n");
        return true;
    }

    static void parse_type(const Instruction & instruction, stream_view & stream )
    {
        //SpvOpTypeArray: type count
        //
/*            SpvOpTypeVoid = 19,
            SpvOpTypeBool = 20,
            SpvOpTypeInt = 21,
            SpvOpTypeFloat = 22,
            SpvOpTypeVector = 23,
            SpvOpTypeMatrix = 24,
            SpvOpTypeImage = 25,
            SpvOpTypeSampler = 26,
            SpvOpTypeSampledImage = 27,
            SpvOpTypeArray = 28,
            SpvOpTypeRuntimeArray = 29,
            SpvOpTypeStruct = 30,
            SpvOpTypeOpaque = 31,
            SpvOpTypePointer = 32,
            SpvOpTypeFunction = 33,
            SpvOpTypeEvent = 34,
            SpvOpTypeDeviceEvent = 35,
            SpvOpTypeReserveId = 36,
            SpvOpTypeQueue = 37,
            SpvOpTypePipe = 38,
            SpvOpTypeForwardPointer = 39,
 */
        uint32_t type;

        for (size_t i = 0; i < instruction.size; i++)
        {
             type = stream.read<uint32_t>();
             printf(" %d", type);
     //       if (instruction.header->op == SpvOpTypeArray)
        //    types.emplace(op, type);
        }
    }
};


#endif