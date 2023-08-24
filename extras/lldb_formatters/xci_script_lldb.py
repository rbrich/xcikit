import lldb


class IndexedMap_SyntheticChildrenProvider:
    def __init__(self, valobj, _dict):
        self.valobj = valobj
        self.m_size = 0
        self.m_chunk = None
        self.chunk_size = 64
        self.chunk_type = None

    def update(self):
        self.m_size = self.valobj.GetChildMemberWithName("m_size").GetValueAsUnsigned()
        self.m_chunk = self.valobj.GetChildMemberWithName("m_chunk")
        #self.chunk_size = self.valobj.GetChildMemberWithName("chunk_size").GetValueAsUnsigned()
        self.chunk_type = self.m_chunk.GetType().template_args[0]

    def num_children(self):
        return self.m_size

    def get_child_at_index(self, index):
        if index < 0 or index >= self.num_children():
            return None
        chunks = self.m_chunk.GetChildMemberWithName("__begin_")  # internal member of std::vector
        chunk = chunks.CreateChildAtOffset("", (index // self.chunk_size) * self.chunk_type.size, self.chunk_type)
        slots = chunk.GetChildMemberWithName("slot")
        slot_type = slots.GetType().GetPointeeType()
        slot = slots.CreateChildAtOffset("", (index % self.chunk_size) * slot_type.size, slot_type)
        elem = slot.GetChildMemberWithName("elem")
        return elem.Clone("[" + str(index) + "]")


def __lldb_init_module(debugger, dict):
    print("Loading xci::script formatters...")
    debugger.HandleCommand("type synthetic add -x '^xci::core::IndexedMap<' --python-class xci_script_lldb.IndexedMap_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type summary add -x '^xci::core::IndexedMap<' --summary-string 'size=${var.m_size}' -w xci")
    debugger.HandleCommand("type category enable xci")

