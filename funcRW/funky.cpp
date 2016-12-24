#define __STDC_FORMAT_MACROS

#include "panda/plugin.h"
#include "panda/plugin_plugin.h"

#include<vector>
#include<string>


using namespace std;



extern "C"{
    // For plugin PRI and liveVarCB
    #include "pri/pri_types.h"
    #include "pri/pri_ext.h"
    #include "pri/pri.h"

    // For anything dwarf
    #include "pri_dwarf/pri_dwarf_types.h"
    #include "pri_dwarf/pri_dwarf_ext.h"

    bool init_plugin(void *);
    void uninit_plugin(void *);
    void on_fn_start(CPUState *cpu, target_ulong pc, const char *filename, const char* functName);
    void on_fn_return(CPUState* cpu, target_ulong pc, const char* filename, const char* functName);
    void pfun(void *var_ty_void, const char *var_nm, LocType loc_t, target_ulong loc, void *in_args);
    int mem_read_callback(CPUState* env, target_ulong pc, target_ulong addr, target_ulong size);
    int virt_mem_read(CPUState* cpu, target_ulong pc, target_ulong addr, target_ulong size, void* buf);
    int mem_write_callback(CPUState *env, target_ulong pc, target_ulong addr, target_ulong size, void *buf);
    int virt_mem_write(CPUState *cpu, target_ulong pc, target_ulong addr, target_ulong size, void *buf);
    int virt_mem_helper(CPUState *cpu, target_ulong pc, target_ulong addr, bool isRead);
}


vector<string> FUNCTION_NAMES;
string currentFunction;
string searchFunction;
uint64_t total_bytes_read, total_bytes_written, total_num_reads, total_num_writes;
uint64_t bytes_read, bytes_written;
uint64_t num_reads, num_writes;
struct args {
    CPUState* cpu;
    const char* src_filename;
    uint64_t src_linenum;
};


bool init_plugin(void *self){
    #if defined(TARGET_I386) && !defined(TARGET_X86_64)
    printf("initializing plugin funky\n");
    searchFunction = "" //MUST BE SET TO WORK
    panda_require("pri");
    assert(init_pri_api());
    panda_require("pri_dwarf");
    assert(init_pri_dwarf_api());

    PPP_REG_CB("pri", on_fn_start, on_fn_start);
    PPP_REG_CB("pri", on_fn_return, on_fn_return);
    { 
        panda_cb pcb;

        panda_enable_precise_pc();
        panda_enable_memcb();
        pcb.virt_mem_before_read = mem_read_callback;
        panda_register_callback(self, PANDA_CB_VIRT_MEM_AFTER_READ, pcb);
        pcb.virt_mem_before_write = mem_write_callback;
        panda_register_callback(self, PANDA_CB_VIRT_MEM_BEFORE_WRITE, pcb);
    }


    #endif
    return true;
}

void uninit_plugin(void *self){
    printf("uninitializing plugin funky\n");
    for(string each:FUNCTION_NAMES){
        printf("%s\n",each.c_str()); 
    }
    printf("-----------------\n      FUNC STATS\n-----------------\n");
    printf("Mem stats: %lu reads, %lu bytes read.\n",
        num_reads, bytes_read
    );
    printf("mem stats: %lu stores, %lu bytes written.\n",
        num_writes, bytes_written       
    );
    printf("-----------------\n      TOTAL STATS\n-----------------\n");
    printf("Mem stats: %lu reads, %lu bytes read.\n",
        total_num_reads, total_bytes_read
    );
    printf("mem stats: %lu stores, %lu bytes written.\n",
        total_num_writes, total_bytes_written       
    );
}

// What's the point of pfun?
void pfun(void* var_ty_void, const char* var_nm, LocType loc_t, target_ulong loc, void* in_args){
    const char* var_ty = dwarf_type_to_string((DwarfVarType *) var_ty_void);

    struct args* args = (struct args *) in_args;
    CPUState* pfun_cpu = args->cpu;
    CPUArchState* pfun_env = (CPUArchState*) pfun_cpu->env_ptr;
}

void on_fn_start(CPUState *cpu, target_ulong pc, const char *filename, const char*functName){
    printf("function start: %s() [%S], pc @0x%x\n", functName, filename, pc);
    
    string functNameString(functName);

    currentFunction = functNameString;
    printf("\n CURR FUNC: %s\n", currentFunction.c_str());
    FUNCTION_NAMES.push_back(functNameString);
    struct args args = {cpu, filename};
    pri_funct_livevar_iter(cpu, pc, (liveVarCB) pfun, (void *) &args);
}

void on_fn_return(CPUState* cpu, target_ulong pc, const char* filename, const char* functName){
    printf("function return: %s() [%S], pc @0x%x\n", functName, filename, pc);
    
    string functNameString(functName);

    printf("\n RETURN FUNC: %s\n", currentFunction.c_str());
    struct args args = {cpu, filename};
    pri_funct_livevar_iter(cpu, pc, (liveVarCB) pfun, (void *) &args);
}


int mem_read_callback(CPUState* env, target_ulong pc, target_ulong addr, target_ulong size){
    if(searchFunction == currentFunction){
        bytes_read += size;
        num_reads++;
    }  
    total_bytes_read += size;
    total_num_reads++;
    return 1;
}

int virt_mem_read(CPUState* cpu, target_ulong pc, target_ulong addr, target_ulong size, void* buf){
    return virt_mem_helper(cpu,pc,addr,false);
}

int virt_mem_helper(CPUState *cpu, target_ulong pc, target_ulong addr, bool isRead){
    SrcInfo info;
    // if NOT in source code, just return
    int rc = pri_get_pc_source_info(cpu, pc, &info);
    // We are not in dwarf info
    if (rc == -1){
        return 0;
    }
    // We are in the first byte of a .plt function
    if (rc == 1) {
        return 0;
    }

    return 0;
}

int mem_write_callback(CPUState *env, target_ulong pc, target_ulong addr, target_ulong size, void *buf){
    if(searchFunction == currentFunction){
        bytes_written += size;
        num_writes++;
    } 
    total_bytes_written+= size;
    total_num_writes++;
    return 1; 
}

int virt_mem_write(CPUState *cpu, target_ulong pc, target_ulong addr, target_ulong size, void *buf) {
    return virt_mem_helper(cpu, pc, addr, false);
}
