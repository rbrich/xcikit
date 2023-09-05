import lldb


class ChunkedStack_SyntheticChildrenProvider:
    def __init__(self, valobj, _dict):
        self.valobj = valobj
        self.buckets = []  # list of (size, Bucket*)

    def update(self):
        tail = self.valobj.GetChildMemberWithName("m_tail")
        bucket = tail.GetChildMemberWithName("next")
        while True:
            size = bucket.GetChildMemberWithName("count").GetValueAsUnsigned()
            self.buckets.append((size, bucket))
            if bucket.GetValueAsUnsigned() == tail.GetValueAsUnsigned():
                break
            bucket = bucket.GetChildMemberWithName("next")

    def num_children(self):
        return sum(size for size, _ in self.buckets)

    def has_children(self):
        # num of elems in first bucket is non-zero
        return self.buckets[0][0] > 0

    def get_child_at_index(self, index):
        if index < 0 or index >= self.num_children():
            return None
        bucket_idx = 0
        elem_idx = index
        while True:
            size, bucket = self.buckets[bucket_idx]
            if elem_idx < size:
                break
            elem_idx -= size
            bucket_idx += 1
        items = bucket.GetChildMemberWithName("items")
        elem = items.GetChildAtIndex(elem_idx, lldb.eNoDynamicValues, True)
        return elem.Clone("[" + str(index) + "]")


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


class TypeInfo_SyntheticChildrenProvider:
    def __init__(self, valobj, _dict):
        self.valobj = valobj
        self.m_type = None
        self.m_is_literal = None
        self.active_member = None

    def update(self):
        union = self.valobj.GetChildAtIndex(0)
        self.m_type = union.GetChildMemberWithName("m_type")
        self.m_is_literal = self.valobj.GetChildMemberWithName("m_is_literal")
        type_ = self.m_type.value
        if type_ == "Unknown":
            active_member_name = "m_var"
        elif type_ in ("List", "Tuple"):
            active_member_name = "m_subtypes"
        elif type_ == "Struct":
            active_member_name = "m_struct_items"
        elif type_ == "Function":
            active_member_name = "m_signature_ptr"
        elif type_ == "Named":
            active_member_name = "m_named_type_ptr"
        else:
            active_member_name = None
            self.active_member = None
        if active_member_name is not None:
            self.active_member = union.GetChildMemberWithName(active_member_name)

    def num_children(self):
        return 2 if self.active_member is None else 3

    def has_children(self):
        return True

    def get_child_index(self, name):
        if name == "m_type":
            index = 0
        elif name == "m_is_literal":
            index = 1
        elif self.active_member is not None and name == self.active_member.name:
            index = -1
        else:
            return None
        if self.active_member is not None:
            index += 1
        return index

    def get_child_at_index(self, index):
        if index < 0 or index >= self.num_children():
            return None
        if self.active_member is not None:
            index -= 1
        if index == 0:
            return self.m_type
        if index == 1:
            return self.m_is_literal
        return self.active_member


def unquote(v):
    return v.removeprefix('"').removesuffix('"')


def TypeInfo_Summary(valobj, _dict, _options):
    m_type = valobj.GetChildMemberWithName('m_type')
    type_ = m_type.value
    if type_ == 'Unknown':
        var = valobj.GetChildMemberWithName("m_var")
        symidx = var.GetChildMemberWithName('m_symidx').GetValueAsUnsigned()
        if symidx != 0xffffffff:
            return var.summary
        return str(type_)
    if type_ == 'List':
        elem_type = valobj.GetChildMemberWithName("m_subtypes").GetChildAtIndex(0).summary
        return f"[{elem_type}]"
    if type_ == 'Tuple':
        subtypes = valobj.GetChildMemberWithName("m_subtypes")
        return f"({', '.join(x.summary for x in subtypes.children)})"
    if type_ == 'Struct':
        struct_items = valobj.GetChildMemberWithName("m_struct_items")
        return f"({', '.join('='.join(unquote(p.summary) for p in x.children) for x in struct_items.children)})"
    if type_ == 'Function':
        signature = valobj.GetChildMemberWithName("m_signature_ptr").Dereference()
        return signature.summary
    if type_ == 'Named':
        named_type = valobj.GetChildMemberWithName('m_named_type_ptr').Dereference()
        return unquote(named_type.GetChildMemberWithName('name').summary)
    return str(type_)


def Signature_Summary(valobj, _dict, _options):
    nonlocals = valobj.GetChildMemberWithName('nonlocals').children
    param_type = valobj.GetChildMemberWithName('param_type')
    return_type = valobj.GetChildMemberWithName('return_type')
    if nonlocals:
        nonlocals_str = f"{{{', '.join(v.summary for v in nonlocals)}}}"
    else:
        nonlocals_str = ""
    return f"{nonlocals_str}{param_type.summary} -> {return_type.summary}"


def SymbolPointer_Summary(valobj, _dict, _options):
    symtab = valobj.GetChildMemberWithName('m_symtab').Dereference()
    symidx = valobj.GetChildMemberWithName('m_symidx').GetValueAsUnsigned()
    symtab_name = symtab.GetChildMemberWithName('m_name')
    m_symbols = symtab.GetChildMemberWithName('m_symbols')
    symbols = m_symbols.GetChildAtIndex(0)
    symbol_type = m_symbols.GetType().template_args[0]
    sym = symbols.CreateChildAtOffset("", symidx * symbol_type.size, symbol_type)
    sym_name = sym.GetChildMemberWithName('m_name')
    return f"{unquote(symtab_name.summary)}::{unquote(sym_name.summary)}"


def __lldb_init_module(debugger, dict):
    print("Loading xci::script formatters...")
    debugger.HandleCommand("type synthetic add -x '^xci::core::ChunkedStack<' --python-class xci_script_lldb.ChunkedStack_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type synthetic add -x '^xci::core::IndexedMap<' --python-class xci_script_lldb.IndexedMap_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type summary add -x '^xci::core::IndexedMap<' --summary-string 'size=${var.m_size}' -w xci")
    debugger.HandleCommand("type synthetic add 'xci::script::TypeInfo' --python-class xci_script_lldb.TypeInfo_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type summary add 'xci::script::TypeInfo' --python-function xci_script_lldb.TypeInfo_Summary -w xci")
    debugger.HandleCommand("type summary add 'xci::script::Signature' --python-function xci_script_lldb.Signature_Summary -w xci")
    debugger.HandleCommand("type summary add 'xci::script::SymbolPointer' --python-function xci_script_lldb.SymbolPointer_Summary -w xci")
    debugger.HandleCommand("type summary add xci::script::MatchScore --summary-string 'exact=${var.m_exact}, coerce=${var.m_coerce}, generic=${var.m_generic}' -w xci")
    debugger.HandleCommand("type category enable xci")

