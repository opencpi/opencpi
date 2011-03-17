OCPI_CDK_DIR=../..
include hdl-pre.mk
HdlError:=warning
define DoFamily
$(info For $(1) Family:)
$(info -> $(call HdlGetFamily,$(1)))
$(info For $(1) Tool:)
$(info -> $(call HdlGetToolSet,$(1)))
endef
x:=$(call DoFamily,123)
y:=$(call DoFamily,virtex5)
z:=$(call DoFamily,isim)
q:=$(call DoFamily,xc6vlx240t)
x:=$(call DoFamily,xilinx)
x:=$(call DoFamily,verilator)
q:=$(call DoFamily,xc5vsx95t)
x: $(call DoFamily,test1)
x: $(call DoFamily,all)


