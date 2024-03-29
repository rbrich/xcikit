# Reference:
# * https://lldb.llvm.org/use/variable.html
# * https://github.com/llvm/llvm-project/blob/main/lldb/examples/synthetic/bitfield/example.py
# * https://melatonin.dev/blog/how-to-create-lldb-type-summaries-and-synthetic-children-for-your-custom-types/

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


class StaticVec_SyntheticChildrenProvider:
    def __init__(self, valobj, _dict):
        self.valobj = valobj
        self.m_size = 0
        self.m_vec = None

    def update(self):
        self.m_size = self.valobj.GetChildMemberWithName("m_size").GetValueAsUnsigned()
        self.m_vec = self.valobj.GetChildMemberWithName("m_vec")

    def num_children(self):
        return self.m_size

    def get_child_at_index(self, index):
        if index < 0 or index >= self.m_size:
            return None
        ptr = self.m_vec.GetChildAtIndex(0).GetChildAtIndex(0).GetChildAtIndex(0)
        elem_type = ptr.GetType().GetPointeeType()
        elem = ptr.CreateChildAtOffset("", index * elem_type.size, elem_type)
        return elem.Clone("[" + str(index) + "]")


class TypeInfo_SyntheticChildrenProvider:
    def __init__(self, valobj, _dict):
        self.valobj = valobj
        self.m_type = None
        self.m_key = None
        self.m_is_literal = None
        self.active_member = None

    def update(self):
        self.m_type = self.valobj.GetChildMemberWithName("m_type")
        self.m_key = self.valobj.GetChildMemberWithName("m_key")
        self.m_is_literal = self.valobj.GetChildMemberWithName("m_is_literal")
        type_ = self.m_type.value
        if type_ == "Unknown":
            active_member_name = "m_var"
        elif type_ in ("List", "Tuple", "Struct"):
            active_member_name = "m_subtypes"
        elif type_ == "Function":
            active_member_name = "m_signature_ptr"
        elif type_ == "Named":
            active_member_name = "m_named_type_ptr"
        else:
            active_member_name = None
            self.active_member = None
        if active_member_name is not None:
            self.active_member = self.valobj.GetChildMemberWithName(active_member_name)

    def num_children(self):
        n = 3  # m_type, m_key, m_is_literal
        if self.active_member is not None:
            n += 1
        return n

    def has_children(self):
        return True

    def get_child_index(self, name):
        if name == "m_type":
            index = 0
        elif name == "m_key":
            index = 1
        elif name == "m_is_literal":
            index = 2
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
            return self.m_key
        if index == 2:
            return self.m_is_literal
        return self.active_member


def unquote(v):
    return v.removeprefix('"').removesuffix('"')


def quote(v):
    return '"' + v + '"'


def NameId_Summary(valobj, _dict, _options):
    pool_mask = 0x8000_0000
    offset_mask = 0x7fff_ffff
    m_id = valobj.GetChildMemberWithName('m_id').GetValueAsUnsigned()
    if (m_id & pool_mask) == pool_mask:
        string_pool = valobj.frame.EvaluateExpression("xci::script::NameId::string_pool()")
        m_strings = string_pool.GetChildMemberWithName('m_strings')
        offset = m_id & offset_mask
        return m_strings.GetChildAtIndex(offset).AddressOf().summary
    else:
        return quote((m_id.to_bytes(4, 'little') + b'\0').split(b'\0', 1)[0].decode('ascii', 'backslashreplace'))


def TypeInfo_Summary(valobj, _dict, _options):
    m_type = valobj.GetChildMemberWithName('m_type').value
    m_key = valobj.GetChildMemberWithName('m_key')
    if m_type == 'Unknown':
        var = valobj.GetChildMemberWithName("m_var")
        symidx = var.GetChildMemberWithName('m_symidx').GetValueAsUnsigned()
        if symidx != 0xffffffff:
            value = var.summary
        else:
            value = str(m_type)
    elif m_type == 'List':
        elem_type = valobj.GetChildMemberWithName("m_subtypes").GetChildAtIndex(0).summary
        value = f"[{elem_type}]"
    elif m_type in ('Tuple', 'Struct'):
        subtypes = valobj.GetChildMemberWithName("m_subtypes")
        value = f"({', '.join(x.summary for x in subtypes.children)})"
    elif m_type == 'Function':
        signature = valobj.GetChildMemberWithName("m_signature_ptr").Dereference()
        value = signature.summary
    elif m_type == 'Named':
        named_type = valobj.GetChildMemberWithName('m_named_type_ptr').Dereference()
        value = unquote(named_type.GetChildMemberWithName('name').summary)
    else:
        value = str(m_type)
    key = unquote(m_key.summary)
    if key:
        return f"{key}={value}"
    return value


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
    p_symtab = valobj.GetChildMemberWithName('m_symtab')
    symidx = valobj.GetChildMemberWithName('m_symidx').GetValueAsUnsigned()
    if p_symtab.GetValueAsUnsigned() == 0 or symidx == 0xFFFF_FFFF:
        return "null"
    symtab = p_symtab.Dereference()
    symtab_name = symtab.GetChildMemberWithName('m_name')
    m_symbols = symtab.GetChildMemberWithName('m_symbols')
    symbols = m_symbols.GetChildAtIndex(0)
    symbol_type = m_symbols.GetType().template_args[0]
    sym = symbols.CreateChildAtOffset("", symidx * symbol_type.size, symbol_type)
    sym_name = sym.GetChildMemberWithName('m_name')
    return f"{unquote(symtab_name.summary)}::{unquote(sym_name.summary)}"


def __lldb_init_module(debugger, _dict):
    print("Loading xci::script formatters...")
    debugger.HandleCommand("type synthetic add -x '^xci::core::ChunkedStack<' --python-class xci_script_lldb.ChunkedStack_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type synthetic add -x '^xci::core::IndexedMap<' --python-class xci_script_lldb.IndexedMap_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type synthetic add -x '^xci::core::StaticVec<' --python-class xci_script_lldb.StaticVec_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type summary add -x '^xci::core::IndexedMap<' --summary-string 'size=${var.m_size}' -w xci")
    debugger.HandleCommand("type synthetic add 'xci::script::TypeInfo' --python-class xci_script_lldb.TypeInfo_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type summary add 'xci::script::NameId' --python-function xci_script_lldb.NameId_Summary -w xci")
    debugger.HandleCommand("type summary add 'xci::script::TypeInfo' --python-function xci_script_lldb.TypeInfo_Summary -w xci")
    debugger.HandleCommand("type summary add 'xci::script::Signature' --python-function xci_script_lldb.Signature_Summary -w xci")
    debugger.HandleCommand("type summary add 'xci::script::SymbolPointer' --python-function xci_script_lldb.SymbolPointer_Summary -w xci")
    debugger.HandleCommand("type summary add xci::script::MatchScore --summary-string 'exact=${var.m_exact}, coerce=${var.m_coerce}, generic=${var.m_generic}' -w xci")
    debugger.HandleCommand("type category enable xci")

