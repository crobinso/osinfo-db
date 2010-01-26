/*
 * osi_api_methods.h
 *
 * Methods provided by OSI
 */

/*******************************************************************************

                   Manipulating the library 

******************************************************************************/

osi_lib_t osi_get_lib_handle(int* err, char* data_dir);
int osi_init_lib(osi_lib_t lib);
int osi_close_lib(osi_lib_t lib);

int osi_set_lib_param(osi_lib_t lib, char* key, char* val);
char* osi_get_lib_param(osi_lib_t lib, char* key, int* err);

int osi_is_error(osi_generic_t t);
int osi_is_null(osi_generic_t t);
int osi_cleanup_handle(osi_generic_t t);

/*******************************************************************************

                   Manipulating hypervisors

 ******************************************************************************/

char** osi_get_all_hypervisor_ids(osi_lib_t lib, int* num, int* err);

osi_hypervisor_t osi_get_hypervisor_by_id(osi_lib_t lib, char* hypervisor_id, int* err);
char* osi_get_hv_id(osi_hypervisor_t hv, int* err);

int osi_set_lib_hypervisor(osi_lib_t lib, char* hypervisor_id);
osi_hypervisor_t osi_get_lib_hypervisor(osi_lib_t lib, int* err);

char** osi_get_hv_device_types(osi_hypervisor_t hv, int* num, int* err);
char** osi_get_hv_all_property_keys(osi_hypervisor_t hv, int* num, int* err);
char** osi_get_hv_property_all_values(osi_hypervisor_t hv, char* propname, int* num, int* err);
char* osi_get_hv_property_first_value(osi_hypervisor_t hv, char* propname, int* err);


/*******************************************************************************

                   Manipulating filters 

******************************************************************************/

osi_filter_t osi_get_filter(osi_lib_t lib, int* err);
int osi_free_filter(osi_filter_t filter);

int osi_add_filter_constraint(osi_filter_t filter, char* propname, char* propval);
int osi_add_relation_constraint(osi_filter_t filter, osi_relationship relationship, osi_os_t os);

int osi_clear_filter_constraint(osi_filter_t filter, char* propname);
int osi_clear_relation_constraint(osi_filter_t filter, osi_relationship relationship);
int osi_clear_all_constraints(osi_filter_t filter);

char** osi_get_filter_constraint_keys(osi_filter_t filter, int* len, int* err);
char* osi_get_filter_constraint_value(osi_filter_t filter, char* propname, int* err);
osi_os_t osi_get_relationship_constraint_value(osi_filter_t filter, osi_relationship relationship, int* err);


/*******************************************************************************

                   Manipulating Devices

 ******************************************************************************/

osi_device_list_t osi_hypervisor_devices(osi_hypervisor_t hv, char* section, osi_filter_t filter, int* err);
osi_device_list_t osi_os_devices(osi_os_t os, char* section, osi_filter_t filter, int* err);
int osi_free_devices_list(osi_device_list_t list);

int osi_devices_list_length(osi_device_list_t list);
osi_device_t osi_get_device_by_index(osi_device_list_t devices, int index, int* err);

char* osi_device_id(osi_device_t device, int* err);
osi_device_t osi_get_device_by_id(osi_lib_t lib, char* device_id, int* err);

char** osi_get_all_device_property_keys(osi_device_t device, int* num, int* err);
char** osi_get_device_property_all_values(osi_device_t device, char* property, int* num, int* err);
char* osi_get_device_property_value(osi_device_t device, char* property, int* err);

char* osi_get_device_driver(osi_device_t device, char* type, osi_os_t os, int* err);
osi_device_t osi_get_preferred_device(osi_os_t os, char* section, osi_filter_t filter, int* err);


/*******************************************************************************

                   Manipulating Operating Systems

 ******************************************************************************/

osi_os_list_t osi_get_os_list(osi_lib_t lib, osi_filter_t filter, int* err);
int osi_free_os_list(osi_os_list_t list);

int osi_os_list_length(osi_os_list_t list);
osi_os_t osi_get_os_by_index(osi_os_list_t list, int index, int* err);

char* osi_get_os_id(osi_os_t os, int* err);
osi_os_t osi_get_os_by_id(osi_lib_t lib, char* id, int* err);

char** osi_get_all_os_property_keys(osi_os_t os, int* num, int* err);
char** osi_get_os_property_all_values(osi_os_t os, char* propname, int* num, int* err);
char* osi_get_os_property_first_value(osi_os_t os, char* propname, int* err);
char** osi_unique_property_values(osi_lib_t lib, char* propname, int* num, int* err);

osi_os_t osi_get_related_os(osi_os_t os, osi_relationship relationship, int* err);
osi_os_list_t osi_unique_relationship_values(osi_lib_t lib, osi_relationship relationship, int* err);

int osi_check_lib(osi_lib_t lib);
int osi_check_hv(osi_hypervisor_t hv);
