Term Ctl (tc)
=============

tabs
----

Alternative to tabs(1).

`tc -t N ...`
Set tab stops every N columns, or N columns apart (when given multiple arguments)

```shell
> tc -t 8
> printf '\ta\tb\tc\td\te\n'
        a       b       c       d       e
> tc -t 4 10 30 4 2
> printf '\ta\tb\tc\td\te\n'
    a         b                             c   d e
```
