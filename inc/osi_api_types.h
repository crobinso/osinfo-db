/*
 * osi_api_types.h
 *
 * Types used by OSI
 */

typedef enum OSI_RELATIONSHIP {
  DERIVES_FROM,
  UPGRADES,
  CLONES
} osi_relationship;

enum OSI_TYPE {
  OSI_LIB_T,
  OSI_HYPERVISOR_T,
  OSI_DEFAULT_HV,
  OSI_OS_T,
  OSI_OS_LIST_T,
  OSI_DEVICE_T,
  OSI_DEVICE_LIST_T,
  OSI_FILTER_T
};

struct osi_generic_type {
  void* backing_object;
  unsigned int initialized;
  enum OSI_TYPE type;
  int error;
};

typedef struct osi_generic_type * osi_generic_t;

typedef osi_generic_t osi_lib_t;
typedef osi_generic_t osi_hypervisor_t;

typedef osi_generic_t osi_os_t;
typedef osi_generic_t osi_os_list_t;

typedef osi_generic_t osi_device_t;
typedef osi_generic_t osi_device_list_t;

typedef osi_generic_t osi_filter_t;
