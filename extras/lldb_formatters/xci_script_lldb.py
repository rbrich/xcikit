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


def unquote(v):
    return v.removeprefix('"').removesuffix('"')


def TypeInfo_Summary(valobj, _dict, _options):
    m_type = valobj.GetChildMemberWithName('m_type')
    m_info = valobj.GetChildMemberWithName('m_info')
    variant_value = m_info.GetChildAtIndex(0)
    type_ = m_type.value
    if type_ == 'Unknown':
        symidx = variant_value.GetChildMemberWithName('m_symidx').GetValueAsUnsigned()
        if symidx != 0xffffffff:
            return variant_value.summary
        return str(type_)
    if type_ == 'List':
        elem_type = variant_value.GetChildAtIndex(0).summary
        return f"[{elem_type}]"
    if type_ == 'Tuple':
        return f"({', '.join(x.summary for x in variant_value.children)})"
    if type_ == 'Struct':
        return f"({', '.join('='.join(unquote(p.summary) for p in x.children) for x in variant_value.children)})"
    if type_ == 'Function':
        signature = variant_value.GetChildAtIndex(0).Dereference()
        return signature.summary
    res = str(type_)
    if variant_value.GetChildAtIndex(0).summary is not None:
        res += f' info={variant_value.GetChildAtIndex(0).summary}'
    return res


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
    # print(sym)
    sym_name = sym.GetChildMemberWithName('m_name')
    return f"{unquote(symtab_name.summary)}::{unquote(sym_name.summary)}"


def __lldb_init_module(debugger, dict):
    print("Loading xci::script formatters...")
    debugger.HandleCommand("type synthetic add -x '^xci::core::IndexedMap<' --python-class xci_script_lldb.IndexedMap_SyntheticChildrenProvider --category xci")
    debugger.HandleCommand("type summary add -x '^xci::core::IndexedMap<' --summary-string 'size=${var.m_size}' -w xci")
    debugger.HandleCommand("type summary add 'xci::script::TypeInfo' --python-function xci_script_lldb.TypeInfo_Summary -w xci")
    debugger.HandleCommand("type summary add 'xci::script::Signature' --python-function xci_script_lldb.Signature_Summary -w xci")
    debugger.HandleCommand("type summary add 'xci::script::SymbolPointer' --python-function xci_script_lldb.SymbolPointer_Summary -w xci")
    debugger.HandleCommand("type summary add xci::script::MatchScore --summary-string 'exact=${var.m_exact}, coerce=${var.m_coerce}, generic=${var.m_generic}' -w xci")
    debugger.HandleCommand("type category enable xci")

