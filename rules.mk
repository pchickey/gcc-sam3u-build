

# gen-target
# 
# $1: target_name
#
define gen-target

$(foreach lib,$(simple_libs),
$(call gen-target-dir-objects,$1,$(lib)))

endef

# gen-target-dir-objects
#
# $1: project target name
# $2: object descriptor (_path, _objs, _cflags) prefix
#
define gen-target-dir-objects
$(call target-dir-objects,$1,$($2_path),$($2_objs),$($2_cflags))
endef

# target-dir-objects #######
#
# $1: project target name
# $2: object file directory
# $3: object files to build
# $4: object file cflags
#
define target-dir-objects

$(eval $1_objs += $$(addprefix $2/,$3))

$(foreach obj,$3,
$(call target-dir-object-cflags,$2/$(obj),$4))

endef

define target-dir-object-cflags
$1: CFLAGS := $(CFLAGS) $2
endef

