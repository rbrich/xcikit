Data Inspector (dati)
=====================

A tool for inspecting binary data files created by `xci::data::BinaryWriter`.

Parses binary data files and shows their content in generic fashion,
with numeric tags and non-blob types converted to human-readable presentation.
Supports data schema, which can additionally name fields and specify their types.

Generate schema file with `fire` tool
-------------------------------------

Prepare schema for Fire modules, compile a module and inspect the module file:
```shell
fire --output-schema /tmp/schema.firm
fire -c examples/script/some.fire -o /tmp/some.firm
dati /tmp/some.firm -s /tmp/firm.schema
```

Alternatively, generate the schema file while compiling the module:
```shell
fire --no-std -c share/script/std.fire -o /tmp/std.firm --output-schema /tmp/firm.schema
dati /tmp/std.firm -s /tmp/firm.schema
```
