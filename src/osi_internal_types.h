/* Forward Declarations */
struct osi_internal_lib;

enum OSI_LIB_STATE {
  UNINITIALIZED,
  INITIALIZED
};

struct osi_value {
  char* val;

  struct list_head list;
};

struct osi_keyval_multi {
  char* key;
  int num_values;
  struct list_head values_list; /* type is osi_value */

  struct list_head list;
};

struct osi_keyval {
  char* key;
  char* value;

  struct list_head list;
};

struct osi_internal_dev {
  char* id;
  int num_params;
  struct list_head param_list; /* type is osi_multi_keyval */

  struct list_head list;

  struct osi_internal_lib * lib;
};

struct osi_device_link {
  struct osi_internal_dev * dev;
  char* driver;
  char* id;

  struct list_head list;
};

struct osi_device_section {
  char* type;

  int num_devices;
  struct list_head devices_list; /* type is osi_device_link */

  struct list_head list;
};

struct osi_internal_hv {
  char* id;

  int num_params;
  struct list_head param_list; /* type is osi_keyval_multi */

  int num_dev_sections;
  struct list_head dev_sections_list; /* type is osi_device_section */

  struct list_head list;

  struct osi_internal_lib * lib;
};

struct osi_os_link {
  /* <subject_os> 'verbs' <direct_object_os>
   * fedora11 upgrades fedora10
   * centos clones rhel
   * scientificlinux derives from rhel
   */
  struct osi_internal_os * subject_os;
  osi_relationship verb;
  struct osi_internal_os * direct_object_os;

  char* dobj_os_id;
  struct list_head list;
};

struct osi_internal_os {
  char* id;
  int num_params;
  struct list_head param_list; /* type is osi_keyval */

  /* Per Hypervisor info for this OS */
  int num_hypervisors;
  struct list_head hypervisor_info_list; /* type is osi_hypervisor_link */

  /* Information for usage with unspecified hypervisor */
  int num_dev_sections;
  struct list_head dev_sections_list; /* type is osi_device_section */

  int num_relationships;
  struct list_head relationships_list; /* type is osi_os_link */

  struct list_head list;

  struct osi_internal_lib * lib;
};

struct osi_filter_relationship {
  struct osi_internal_os * os;
  osi_relationship relationship;

  struct list_head list;
};

struct osi_hypervisor_link {
  struct osi_internal_hv * hv;
  struct osi_internal_os * os;

  char* hv_id;

  int num_dev_sections;
  struct list_head dev_sections_list; /* type is osi_device_section */

  struct list_head list;
};

struct osi_internal_lib {
  enum OSI_LIB_STATE lib_state;
  char* libvirt_version;
  char* backing_data_location;

  int num_params;
  struct list_head param_list; /* type is osi_keyval */

  int num_hypervisors;
  struct list_head hypervisor_list; /* type is osi_internal_hv */
  struct osi_internal_hv * lib_hypervisor;

  int num_devices;
  struct list_head device_list; /* type is osi_internal_dev */

  int num_os;
  struct list_head os_list; /* type is osi_internal_os */
};

struct osi_internal_filter {
  int num_constraints;
  struct list_head constraints_list; /* type is osi_keyval */

  int num_relationship_constraints;
  struct list_head relationship_constraints_list; /* type is osi_filter_relationship */

  struct osi_internal_lib * lib;
};

struct osi_dev_list {
  int length;
  struct osi_internal_dev ** dev_refs;
};

struct osi_os_list {
  int length;
  struct osi_internal_os ** os_refs;
};

void __osi_free_keyval(struct osi_keyval_multi * kv);
void __osi_free_device_section(struct osi_device_section * section);
struct osi_keyval_multi * __osi_find_kv(char* key, struct list_head * search_list);
struct osi_keyval_multi * __osi_create_kv(char* key, struct list_head * dest_list);
struct osi_keyval_multi * __osi_find_or_create_kv(char* key, struct list_head * list, int* num);
int __osi_add_keyval_multi_value(struct osi_keyval_multi * kv, char* value);
int __osi_store_keyval(char* key, char* value, struct list_head * list, int* num);
void __osi_free_hv(struct osi_internal_hv * hv);
void __osi_free_device(struct osi_internal_dev * dev);
void __osi_free_os(struct osi_internal_os * os);
void __osi_free_kv_pair(struct osi_keyval * kv_pair);
void __osi_free_lib(struct osi_internal_lib * lib);
void __osi_free_hypervisor_link(struct osi_hypervisor_link * hv_link);
struct osi_internal_dev * __osi_find_dev_by_id(struct osi_internal_lib * lib, char* id);
struct osi_internal_hv * __osi_find_hv_by_id(struct osi_internal_lib * lib, char* id);
void __osi_free_os_link(struct osi_os_link * os_link);
struct osi_internal_os * __osi_find_os_by_id(struct osi_internal_lib * lib, char* id);
void __osi_free_filter_relationship(struct osi_filter_relationship * fr);
