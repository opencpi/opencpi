#
# This file was automatically generated from CosNaming.idl
# by idl2tcl. Do not edit.
#

package require combat

combat::ir add \
{{module {IDL:omg.org/CosNaming:1.0 CosNaming 1.0} {{typedef\
{IDL:omg.org/CosNaming/Istring:1.0 Istring 1.0} {string 0}} {struct\
{IDL:omg.org/CosNaming/NameComponent:1.0 NameComponent 1.0} {{id\
IDL:omg.org/CosNaming/Istring:1.0} {kind IDL:omg.org/CosNaming/Istring:1.0}}\
{}} {typedef {IDL:omg.org/CosNaming/Name:1.0 Name 1.0} {sequence\
IDL:omg.org/CosNaming/NameComponent:1.0}} {enum\
{IDL:omg.org/CosNaming/BindingType:1.0 BindingType 1.0} {nobject ncontext}}\
{struct {IDL:omg.org/CosNaming/Binding:1.0 Binding 1.0} {{binding_name\
IDL:omg.org/CosNaming/Name:1.0} {binding_type\
IDL:omg.org/CosNaming/BindingType:1.0}} {}} {typedef\
{IDL:omg.org/CosNaming/BindingList:1.0 BindingList 1.0} {sequence\
IDL:omg.org/CosNaming/Binding:1.0}} {interface\
{IDL:omg.org/CosNaming/BindingIterator:1.0 BindingIterator 1.0} {}\
{{operation {IDL:omg.org/CosNaming/BindingIterator/next_one:1.0 next_one 1.0}\
boolean {{out b IDL:omg.org/CosNaming/Binding:1.0}} {}} {operation\
{IDL:omg.org/CosNaming/BindingIterator/destroy:1.0 destroy 1.0} void {} {}}\
{operation {IDL:omg.org/CosNaming/BindingIterator/next_n:1.0 next_n 1.0}\
boolean {{in how_many {unsigned long}} {out bl\
IDL:omg.org/CosNaming/BindingList:1.0}} {}}}} {interface\
{IDL:omg.org/CosNaming/NamingContext:1.0 NamingContext 1.0} {} {{enum\
{IDL:omg.org/CosNaming/NamingContext/NotFoundReason:1.0 NotFoundReason 1.0}\
{missing_node not_context not_object}} {exception\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0 NotFound 1.0} {{why\
IDL:omg.org/CosNaming/NamingContext/NotFoundReason:1.0} {rest_of_name\
IDL:omg.org/CosNaming/Name:1.0}} {}} {exception\
{IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0 CannotProceed 1.0}\
{{cxt IDL:omg.org/CosNaming/NamingContext:1.0} {rest_of_name\
IDL:omg.org/CosNaming/Name:1.0}} {}} {exception\
{IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0 InvalidName 1.0} {} {}}\
{exception {IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0 AlreadyBound\
1.0} {} {}} {exception {IDL:omg.org/CosNaming/NamingContext/NotEmpty:1.0\
NotEmpty 1.0} {} {}} {operation {IDL:omg.org/CosNaming/NamingContext/bind:1.0\
bind 1.0} void {{in n IDL:omg.org/CosNaming/Name:1.0} {in obj Object}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/unbind:1.0 unbind 1.0} void {{in n\
IDL:omg.org/CosNaming/Name:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/resolve:1.0 resolve 1.0} Object {{in n\
IDL:omg.org/CosNaming/Name:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/list:1.0 list 1.0} void {{in how_many\
{unsigned long}} {out bl IDL:omg.org/CosNaming/BindingList:1.0} {out bi\
IDL:omg.org/CosNaming/BindingIterator:1.0}} {}} {operation\
{IDL:omg.org/CosNaming/NamingContext/rebind_context:1.0 rebind_context 1.0}\
void {{in n IDL:omg.org/CosNaming/Name:1.0} {in nc\
IDL:omg.org/CosNaming/NamingContext:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/destroy:1.0 destroy 1.0} void {}\
IDL:omg.org/CosNaming/NamingContext/NotEmpty:1.0} {operation\
{IDL:omg.org/CosNaming/NamingContext/bind_context:1.0 bind_context 1.0} void\
{{in n IDL:omg.org/CosNaming/Name:1.0} {in nc\
IDL:omg.org/CosNaming/NamingContext:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/bind_new_context:1.0 bind_new_context\
1.0} IDL:omg.org/CosNaming/NamingContext:1.0 {{in n\
IDL:omg.org/CosNaming/Name:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/rebind:1.0 rebind 1.0} void {{in n\
IDL:omg.org/CosNaming/Name:1.0} {in obj Object}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/new_context:1.0 new_context 1.0}\
IDL:omg.org/CosNaming/NamingContext:1.0 {} {}}}} {interface\
{IDL:omg.org/CosNaming/NamingContextExt:1.0 NamingContextExt 1.0}\
IDL:omg.org/CosNaming/NamingContext:1.0 {{typedef\
{IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0 StringName 1.0}\
{string 0}} {typedef {IDL:omg.org/CosNaming/NamingContextExt/Address:1.0\
Address 1.0} {string 0}} {typedef\
{IDL:omg.org/CosNaming/NamingContextExt/URLString:1.0 URLString 1.0} {string\
0}} {exception {IDL:omg.org/CosNaming/NamingContextExt/InvalidAddress:1.0\
InvalidAddress 1.0} {} {}} {operation\
{IDL:omg.org/CosNaming/NamingContextExt/to_string:1.0 to_string 1.0}\
IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0 {{in n\
IDL:omg.org/CosNaming/Name:1.0}}\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0} {operation\
{IDL:omg.org/CosNaming/NamingContextExt/resolve_str:1.0 resolve_str 1.0}\
Object {{in n IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContextExt/to_url:1.0 to_url 1.0}\
IDL:omg.org/CosNaming/NamingContextExt/URLString:1.0 {{in addr\
IDL:omg.org/CosNaming/NamingContextExt/Address:1.0} {in sn\
IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0}}\
{IDL:omg.org/CosNaming/NamingContextExt/InvalidAddress:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContextExt/to_name:1.0 to_name 1.0}\
IDL:omg.org/CosNaming/Name:1.0 {{in sn\
IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0}}\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0}}}}}}

