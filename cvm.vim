syntax clear

syntax keyword cvm_keyword var const
highlight link cvm_keyword Keyword

syntax keyword cvm_if if if-else
highlight link cvm_if Conditional

syntax keyword cvm_loop while
highlight link cvm_loop Repeat

syntax keyword cvm_builtin over add sub drop swap dup gt lt geq leq eq neq puti puts
highlight link cvm_builtin Function

syntax region cvm_comment start=/</ end=/>/ contains=cvm_comment
highlight link cvm_comment Comment

syntax region cvm_str_literal start=/"/ skip=/\\"/ end=/"/
highlight link cvm_str_literal String

syntax match cvm_int_literal "\<\d\+\>"
highlight link cvm_int_literal Number

syntax match cvm_definition /{:\zs\w\+/
highlight link cvm_definition Define

syntax keyword cvm_bool true false
highlight link cvm_bool Boolean
