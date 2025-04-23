The folder `routing` was removed due to incompatibility error.
When running `waf` with --enable-examples, `static-routing-slash32.cc` raises an error related to DropTailQueue not being a template, which it is in version 3.29, but in the implementation here it is not.
