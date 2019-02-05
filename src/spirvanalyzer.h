#ifndef __SpirvAnalyzer_h__
#define __SpirvAnalyzer_h__


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
    static const char * storage2str(StorageClass s)
    {
        switch (s)
        {
            case StorageClassUniformConstant: return "StorageClassUniformConstant";
            case StorageClassInput:           return "StorageClassInput";
            case StorageClassUniform:         return "StorageClassUniform";
            case StorageClassOutput:          return "StorageClassOutput";
            case StorageClassWorkgroup:       return "StorageClassWorkgroup";
            case StorageClassCrossWorkgroup:  return "StorageClassCrossWorkgroup";
            case StorageClassPrivate:         return "StorageClassPrivate";
            case StorageClassFunction:        return "StorageClassFunction";
            case StorageClassGeneric:         return "StorageClassGeneric";
            case StorageClassPushConstant:    return "StorageClassPushConstant";
            case StorageClassAtomicCounter:   return "StorageClassAtomicCounter";
            case StorageClassImage:           return "StorageClassImage";
            case StorageClassStorageBuffer:   return "StorageClassStorageBuffer";
        }
        return "unknown storage";
    }


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
     //   SpvOp               op:8;    //uint16_t opcode : 16; //
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

        template <class T> const T *    shift() const                       { const T * ret = (const T*)(m_data); m_data += sizeof(T); return ret; }

        void                            read(void * data, uint32_t size)    { memcpy(data, m_data, size); m_data += size; }
        const char *                    data() const                        { return m_currptr; }

        void                            seek(size_t pos)                    { m_data = m_currptr + pos; }
        size_t                          tell() const                        { return m_data - m_currptr; }
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

        std::string                     name;
        uint32_t                        id = UINT32_MAX;
        uint32_t                        description = UINT32_MAX;
        uint32_t                        binding = UINT32_MAX;
        uint32_t                        location = UINT32_MAX;

        std::vector<Decorate>           m_childs;
    };
public:
    static bool analyze(const void * data, size_t size)
    {
        stream_view stream((const char *)data, size);

        const SpirvHeader * h = stream.shift<SpirvHeader>();
        if (h->magic != SPIRV_MAGIC || h->version != SPIRV_VERSION)
            return false;

        uint16_t bufer[256] = {};
        char    cbufer[256] = {};

        std::map<uint32_t, Decorate> decorates;
        std::vector<Instruction> instructions;
        size_t count = size - sizeof(SpirvHeader);
        while (!stream.eof())
        {
            size_t ss = sizeof(InstructionHeader);
            InstructionHeader * instr = (InstructionHeader*)stream.shift<InstructionHeader>();
            instructions.emplace_back(instr, instr->count - 1, stream.tell());

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
            switch (instruction.header->op)
            {
                case 5: // name
                {
                    uint32_t id = 0;
                    stream.seek(instruction.offset);
                    stream.read(&id, sizeof(uint32_t));
                    stream.read(buff, sizeof(uint32_t)* instruction.size - 1);
                    size_t len = strlen(buff);

                    auto it = decorates.find(id);
                    if (it == decorates.end() && len > 0)
                        decorates.insert(std::make_pair(id, Decorate(id, buff)));
                } break;

                case 6: //member name
                {
                    uint32_t id = 0; uint32_t mid;
                    stream.read(&id, sizeof(uint32_t));
                    stream.read(&mid, sizeof(uint32_t));
                    stream.read(buff, sizeof(uint32_t)* instruction.size - 1);

                    auto it = decorates.find(id);
                    it->second.add(Decorate(mid, buff));
                }break;

                case SpvOpDecorate: // 71:// decorate
                {
                    uint32_t id = 0; uint32_t mid; uint32_t data = 0;
                    stream.read(&id, sizeof(uint32_t));
                    stream.read(&mid, sizeof(uint32_t));
                    auto it = decorates.find(id);
                    for (size_t j = 0; j < instruction.size - 2; ++j)
                    {
                        stream.read(&data, sizeof(uint32_t));
                        if (mid == DecorationLocation) it->second.location = data;
                        if (mid == DecorationBinding)  it->second.binding = data;
                        if (mid == DecorationDescriptorSet) it->second.description = data;
                    }
                }break;

                case SpvOpMemberDecorate: // 72: // member decorate
                {
                    uint32_t id = 0; uint32_t mid;
                    SpvDecoration decor; 
                    SpvBuiltIn builtin;
                    stream.read(&id, sizeof(uint32_t));
                    stream.read(&mid, sizeof(uint32_t));
                    stream.read(&decor, sizeof(uint32_t)); // 11 = BuiltIn
                    stream.read(&builtin, sizeof(uint32_t));

                    auto it = decorates.find(id);
                    if (it != decorates.end())
                    {
                        printf("\nmember decorate: %d(%s) %d %d %d", id, it->second.name.c_str(), mid, decor, builtin);
                    }
                  //  printf("\nmember decorate: %d %d %d %d", id, mid, id0, mid0);
                }break;

                case SpvOpTypePointer:
                {
                    uint32_t tokens[4] = {};
                    stream.read(&tokens, sizeof(uint32_t)*instruction.size);
                    auto it = decorates.find(tokens[0]);
                    printf("\nSpvOpTypePointer [%d] [%d] [%d] [%d]", tokens[0], tokens[1], tokens[2], tokens[3]);
                }break;
                case SpvOpTypeStruct:
                {
                                        uint32_t id, type;
                    stream.read(&id, sizeof(uint32_t));
                    stream.read(&type, sizeof(uint32_t));
                    auto it = decorates.find(id);
                    printf("");
                }break;

                case SpvOpVariable: // 59://variable 
                {
                    uint32_t type, id;
                    StorageClass storage;
                    uint32_t initializer = 0;
                    stream.read(&type, sizeof(uint32_t));
                    stream.read(&id, sizeof(uint32_t));
                    stream.read(&storage, sizeof(uint32_t));
                    instruction.size == 4 ? stream.read(&initializer, sizeof(uint32_t)) : 0;
                    auto it = decorates.find(id);

                    if (it != decorates.end())
                        printf("\nvariable: %d(%s) %d %s", type, it->second.name.c_str(), id, storage2str(storage));
                    else
                        printf("\nvariable: %d %d %s", type, id, storage2str(storage));
                }break;
            }
        }
        return true;
    }
};


#endif