widechar_width
==============

[GitHub repo](https://github.com/ridiculousfish/widecharwidth)

Computing width of strings in terminal is not easy. Normal ASCII characters are one column wide,
but some unicode characters span two columns, and some are invisible (zero columns).

This is an implementation of non-standardized `wcwidth` function. Even though that function is sometimes
found in `<wchar.h>`, it's usually old, or it may be missing completely. Thus, we need a third party
implementation.

Local changes:

* split to cpp/h (I don't want to recompile this each time I do a change elsewhere)

* change the anonymous namespace to `wcw` (I like third-party libs namespaced)

* change `uint32_t` -> `char32_t`
