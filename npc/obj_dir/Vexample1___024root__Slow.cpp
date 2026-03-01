// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vexample1.h for the primary calling header

#include "Vexample1__pch.h"

void Vexample1___024root___ctor_var_reset(Vexample1___024root* vlSelf);

Vexample1___024root::Vexample1___024root(Vexample1__Syms* symsp, const char* namep)
 {
    vlSymsp = symsp;
    vlNamep = strdup(namep);
    // Reset structure values
    Vexample1___024root___ctor_var_reset(this);
}

void Vexample1___024root::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Vexample1___024root::~Vexample1___024root() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
