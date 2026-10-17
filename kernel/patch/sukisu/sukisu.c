#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <hook.h>
#include <symbol.h>
#include <kconfig.h>
#include <common.h>
#include <module.h>
#include <predata.h>

static long call_kpm_control(const char *name, const char * args, long arg_len, void *__user out_msg, int outlen)
{
    return module_control0(name, arg_len <= 0 ? 0 : args, out_msg, outlen);
}

static long call_kpm_list(char * buf, int len)
{
    int sz = list_modules(buf, len);
    return sz;
}

// =====================================================================================

void before_sukisu_load_module_path(hook_fargs4_t* args, void* udata) {
    const char* path = (const char*) args->arg0;
    const char* arg = (const char*) args->arg1;
    void* ptr = (void*) args->arg2;
    int* result = (void*) args->arg3;

    logkfi("Load KPM: %s", path);

    *result = (int) load_module_path(path, arg, ptr);
    args->skip_origin = 1;
}

void before_sukisu_unload_module(hook_fargs3_t* args,void* udata) {
    const char* name = (const char*)args->arg0;
    void* ptr = (void*) args->arg1;
    int* result = (void*) args->arg2;
    *result = (int) unload_module(name, ptr);
    args->skip_origin = 1;
}

void before_sukisu_kpm_num(hook_fargs1_t* args, void* udata) {
    int* result = (void*) args->arg0;

    *result = (int) get_module_nums();
    args->skip_origin = 1;
}

void before_sukisu_kpm_list(hook_fargs3_t* args, void* udata) {
    char* out = (char* __user) args->arg0;
    int len = (int) args->arg1;
    int * result = (void*) args->arg2;

    int res = (int) call_kpm_list(out, len);
    
    *result = res;
    args->skip_origin = 1;
}

void before_sukisu_kpm_info(hook_fargs3_t* args, void* udata) {
    char* name = (char*) args->arg0;
    char* buf = (char*) args->arg1;
    int buf_size = (int) args->arg2;
    int* size = (void*) args->arg3;
    *size = get_module_info(name, buf, buf_size);
    args->skip_origin = 1;
}

void before_sukisu_kpm_version(hook_fargs3_t* args, void* udata) {
    char * buf = (char *) args->arg0;
    int buf_size =  (int) args->arg1;
    const char *buildtime = get_build_time();

    snprintf(buf, buf_size-1, "%d (%s)", kpver, buildtime);
    args->skip_origin = 1;
}

void before_sukisu_kpm_control(hook_fargs3_t* args, void* udata) {
    const char * name = (const char *) args->arg0;
    const char * arg = (const char *) args->arg1;
    long arg_len = (long) args->arg2;
    int * result = (void*) args->arg3;
    int res = (int) call_kpm_control(name, arg, arg_len, NULL, 0);

    *result = res;
    args->skip_origin = 1;
}

void init_sukisu_ultra() {
    unsigned long addr;
    int rc;

    addr = kallsyms_lookup_name("sukisu_kpm_load_module_path");
    if(addr) {
        rc = hook_wrap4((void*) addr, before_sukisu_load_module_path, NULL, NULL);
        log_boot("hook sukisu_load_module_path rc:%d \n", rc);
    } else {
        log_boot("hook sukisu_load_module_path faild \n", rc);
    }

    addr = kallsyms_lookup_name("sukisu_kpm_unload_module");
    if(addr) {
        rc = hook_wrap3((void*) addr, before_sukisu_unload_module, NULL, NULL);
        log_boot("hook sukisu_kpm_unload_module rc:%d \n", rc);
    } else {
        log_boot("hook sukisu_kpm_unload_module faild \n", rc);
    }

    addr = kallsyms_lookup_name("sukisu_kpm_num");
    if(addr) {
        rc = hook_wrap1((void*) addr, before_sukisu_kpm_num, NULL, NULL);
        log_boot("hook sukisu_kpm_num rc:%d \n", rc);
    } else {
        log_boot("hook sukisu_kpm_num faild \n", rc);
    }

    addr = kallsyms_lookup_name("sukisu_kpm_list");
    if(addr) {
        rc = hook_wrap3((void*) addr, before_sukisu_kpm_list, NULL, NULL);
        log_boot("hook sukisu_kpm_list rc:%d \n", rc);
    } else {
        log_boot("hook sukisu_kpm_list faild \n", rc);
    }

    addr = kallsyms_lookup_name("sukisu_kpm_info");
    if(addr) {
        rc = hook_wrap3((void*) addr, before_sukisu_kpm_info, NULL, NULL);
        log_boot("hook sukisu_kpm_info rc:%d \n", rc);
    } else {
        log_boot("hook sukisu_kpm_info faild \n", rc);
    }

    addr = kallsyms_lookup_name("sukisu_kpm_control");
    if(addr) {
        rc = hook_wrap3((void*) addr, before_sukisu_kpm_control, NULL, NULL);
        log_boot("hook sukisu_kpm_control rc:%d \n", rc);
    } else {
        log_boot("hook sukisu_kpm_control faild \n", rc);
    }

    addr = kallsyms_lookup_name("sukisu_kpm_version");
    if(addr) {
        rc = hook_wrap3((void*) addr, before_sukisu_kpm_version, NULL, NULL);
        log_boot("hook sukisu_kpm_version rc:%d \n", rc);
    } else {
        log_boot("hook sukisu_kpm_version faild \n", rc);
    }

}
EXPORT_SYMBOL(init_sukisu_ultra);
KP_EXPORT_SYMBOL(init_sukisu_ultra);