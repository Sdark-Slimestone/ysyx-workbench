// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vexample1__pch.h"
#include "verilated_vcd_c.h"

//============================================================
// Constructors

Vexample1::Vexample1(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Vexample1__Syms(contextp(), _vcname__, this)}
    , a{vlSymsp->TOP.a}
    , b{vlSymsp->TOP.b}
    , f{vlSymsp->TOP.f}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
    contextp()->traceBaseModelCbAdd(
        [this](VerilatedTraceBaseC* tfp, int levels, int options) { traceBaseModel(tfp, levels, options); });
}

Vexample1::Vexample1(const char* _vcname__)
    : Vexample1(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Vexample1::~Vexample1() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Vexample1___024root___eval_debug_assertions(Vexample1___024root* vlSelf);
#endif  // VL_DEBUG
void Vexample1___024root___eval_static(Vexample1___024root* vlSelf);
void Vexample1___024root___eval_initial(Vexample1___024root* vlSelf);
void Vexample1___024root___eval_settle(Vexample1___024root* vlSelf);
void Vexample1___024root___eval(Vexample1___024root* vlSelf);

void Vexample1::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vexample1::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vexample1___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_activity = true;
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Vexample1___024root___eval_static(&(vlSymsp->TOP));
        Vexample1___024root___eval_initial(&(vlSymsp->TOP));
        Vexample1___024root___eval_settle(&(vlSymsp->TOP));
        vlSymsp->__Vm_didInit = true;
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Vexample1___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Vexample1::eventsPending() { return false; }

uint64_t Vexample1::nextTimeSlot() {
    VL_FATAL_MT(__FILE__, __LINE__, "", "No delays in the design");
    return 0;
}

//============================================================
// Utilities

const char* Vexample1::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Vexample1___024root___eval_final(Vexample1___024root* vlSelf);

VL_ATTR_COLD void Vexample1::final() {
    Vexample1___024root___eval_final(&(vlSymsp->TOP));
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Vexample1::hierName() const { return vlSymsp->name(); }
const char* Vexample1::modelName() const { return "Vexample1"; }
unsigned Vexample1::threads() const { return 1; }
void Vexample1::prepareClone() const { contextp()->prepareClone(); }
void Vexample1::atClone() const {
    contextp()->threadPoolpOnClone();
}
std::unique_ptr<VerilatedTraceConfig> Vexample1::traceConfig() const {
    return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{false, false, false}};
};

//============================================================
// Trace configuration

void Vexample1___024root__trace_decl_types(VerilatedVcd* tracep);

void Vexample1___024root__trace_init_top(Vexample1___024root* vlSelf, VerilatedVcd* tracep);

VL_ATTR_COLD static void trace_init(void* voidSelf, VerilatedVcd* tracep, uint32_t code) {
    // Callback from tracep->open()
    Vexample1___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vexample1___024root*>(voidSelf);
    Vexample1__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {
        VL_FATAL_MT(__FILE__, __LINE__, __FILE__,
            "Turning on wave traces requires Verilated::traceEverOn(true) call before time 0.");
    }
    vlSymsp->__Vm_baseCode = code;
    tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);
    Vexample1___024root__trace_decl_types(tracep);
    Vexample1___024root__trace_init_top(vlSelf, tracep);
    tracep->popPrefix();
}

VL_ATTR_COLD void Vexample1___024root__trace_register(Vexample1___024root* vlSelf, VerilatedVcd* tracep);

VL_ATTR_COLD void Vexample1::traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options) {
    (void)levels; (void)options;
    VerilatedVcdC* const stfp = dynamic_cast<VerilatedVcdC*>(tfp);
    if (VL_UNLIKELY(!stfp)) {
        vl_fatal(__FILE__, __LINE__, __FILE__,"'Vexample1::trace()' called on non-VerilatedVcdC object;"
            " use --trace-fst with VerilatedFst object, and --trace-vcd with VerilatedVcd object");
    }
    stfp->spTrace()->addModel(this);
    stfp->spTrace()->addInitCb(&trace_init, &(vlSymsp->TOP), name(), false, 3);
    Vexample1___024root__trace_register(&(vlSymsp->TOP), stfp->spTrace());
}
