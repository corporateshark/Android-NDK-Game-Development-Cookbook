#
# Part of the Linderdaum Engine

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

GLOBAL_C_INCLUDES := $(LOCAL_PATH)

GLOBAL_C_INCLUDES += \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../ode \
	$(LOCAL_PATH)/../ode/src \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../OPCODE \

LOCAL_SRC_FILES += \
	../ode/src/array.cpp \
	../ode/src/box.cpp \
	../ode/src/capsule.cpp \
	../ode/src/collision_cylinder_box.cpp \
	../ode/src/collision_cylinder_plane.cpp \
	../ode/src/collision_cylinder_sphere.cpp \
	../ode/src/collision_cylinder_trimesh.cpp \
	../ode/src/collision_kernel.cpp \
	../ode/src/collision_quadtreespace.cpp \
	../ode/src/collision_sapspace.cpp \
	../ode/src/collision_space.cpp \
	../ode/src/collision_transform.cpp \
	../ode/src/collision_trimesh_box.cpp \
	../ode/src/collision_trimesh_ccylinder.cpp \
	../ode/src/collision_trimesh_disabled.cpp \
	../ode/src/collision_trimesh_distance.cpp \
	../ode/src/collision_trimesh_opcode.cpp \
	../ode/src/collision_trimesh_plane.cpp \
	../ode/src/collision_trimesh_ray.cpp \
	../ode/src/collision_trimesh_sphere.cpp \
	../ode/src/collision_trimesh_trimesh.cpp \
	../ode/src/collision_trimesh_trimesh_new.cpp \
	../ode/src/collision_util.cpp \
	../ode/src/convex.cpp \
	../ode/src/cylinder.cpp \
	../ode/src/error.cpp \
	../ode/src/export-dif.cpp \
	../ode/src/fastdot.c \
	../ode/src/fastldlt.c \
	../ode/src/fastlsolve.c \
	../ode/src/fastltsolve.c \
	../ode/src/heightfield.cpp \
	../ode/src/joints/amotor.cpp \
	../ode/src/joints/ball.cpp \
	../ode/src/joints/contact.cpp \
	../ode/src/joints/fixed.cpp \
	../ode/src/joints/hinge.cpp \
	../ode/src/joints/hinge2.cpp \
	../ode/src/joints/joint.cpp \
	../ode/src/joints/lmotor.cpp \
	../ode/src/joints/null.cpp \
	../ode/src/joints/piston.cpp \
	../ode/src/joints/plane2d.cpp \
	../ode/src/joints/pr.cpp \
	../ode/src/joints/pu.cpp \
	../ode/src/joints/slider.cpp \
	../ode/src/joints/universal.cpp \
	../ode/src/lcp.cpp \
	../ode/src/mass.cpp \
	../ode/src/mat.cpp \
	../ode/src/matrix.cpp \
	../ode/src/memory.cpp \
	../ode/src/misc.cpp \
	../ode/src/nextafterf.c \
	../ode/src/obstack.cpp \
	../ode/src/ode.cpp \
	../ode/src/odeinit.cpp \
	../ode/src/odemath.cpp \
	../ode/src/odeou.cpp \
	../ode/src/odetls.cpp \
	../ode/src/plane.cpp \
	../ode/src/quickstep.cpp \
	../ode/src/ray.cpp \
	../ode/src/rotation.cpp \
	../ode/src/sphere.cpp \
	../ode/src/step.cpp \
	../ode/src/timer.cpp \
	../ode/src/util.cpp \
	../OPCODE/Ice/IceAABB.cpp \
	../OPCODE/Ice/IceContainer.cpp \
	../OPCODE/Ice/IceHPoint.cpp \
	../OPCODE/Ice/IceIndexedTriangle.cpp \
	../OPCODE/Ice/IceMatrix3x3.cpp \
	../OPCODE/Ice/IceMatrix4x4.cpp \
	../OPCODE/Ice/IceOBB.cpp \
	../OPCODE/Ice/IcePlane.cpp \
	../OPCODE/Ice/IcePoint.cpp \
	../OPCODE/Ice/IceRandom.cpp \
	../OPCODE/Ice/IceRay.cpp \
	../OPCODE/Ice/IceRevisitedRadix.cpp \
	../OPCODE/Ice/IceSegment.cpp \
	../OPCODE/Ice/IceTriangle.cpp \
	../OPCODE/Ice/IceUtils.cpp \
	../OPCODE/Opcode.cpp \
	../OPCODE/OPC_AABBCollider.cpp \
	../OPCODE/OPC_AABBTree.cpp \
	../OPCODE/OPC_BaseModel.cpp \
	../OPCODE/OPC_Collider.cpp \
	../OPCODE/OPC_Common.cpp \
	../OPCODE/OPC_HybridModel.cpp \
	../OPCODE/OPC_LSSCollider.cpp \
	../OPCODE/OPC_MeshInterface.cpp \
	../OPCODE/OPC_Model.cpp \
	../OPCODE/OPC_OBBCollider.cpp \
	../OPCODE/OPC_OptimizedTree.cpp \
	../OPCODE/OPC_Picking.cpp \
	../OPCODE/OPC_PlanesCollider.cpp \
	../OPCODE/OPC_RayCollider.cpp \
	../OPCODE/OPC_SphereCollider.cpp \
	../OPCODE/OPC_TreeBuilders.cpp \
	../OPCODE/OPC_TreeCollider.cpp \
	../OPCODE/OPC_VolumeCollider.cpp \

LOCAL_MODULE := ODE

GLOBAL_CFLAGS   := -O3 -Werror -DHAVE_CONFIG_H=1 -DODE_LIB
GLOBAL_CFLAGS   += -Wno-switch -Wno-deprecated-writable-strings -Wno-null-dereference

ifeq ($(TARGET_ARCH),x86)
	LOCAL_CFLAGS   := $(GLOBAL_CFLAGS)
else
	LOCAL_CFLAGS   := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums $(GLOBAL_CFLAGS)
endif

LOCAL_C_INCLUDES := $(GLOBAL_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)
