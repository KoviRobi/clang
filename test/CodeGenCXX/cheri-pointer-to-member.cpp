// RUN: %clang %s -mabi=purecap -fno-rtti -std=c++11 -target cheri-unknown-freebsd -o - -emit-llvm -S | FileCheck %s
// RUN: %clang %s -mabi=n64 -fno-rtti -std=c++11 -target cheri-unknown-freebsd -o - -emit-llvm -S -O2 | FileCheck %s -check-prefix N64
// RUN: %clang %s -mabi=n64 -fno-rtti -std=c++11 -target cheri-unknown-freebsd -o - -S -O2 | FileCheck %s -check-prefix N64-ASM

class A {
public:
  int x = 3;
  int y = 4;
  int foo() { return 1; }
  virtual int foo_virtual() { return 2; }
  int bar() { return 1; }
  virtual int bar_virtual() { return 2; }
};

// compare IR with simulated function pointers:
struct mem_fn_ptr {
  void* ptr;
  long offset;
};
void func(void) {}
mem_fn_ptr virt = { (void*)32, 1 };
mem_fn_ptr nonvirt = { (void*)&func, 1 };
// CHECK: @virt = addrspace(200) global { i8 addrspace(200)*, i64 } { i8 addrspace(200)* inttoptr (i64 32 to i8 addrspace(200)*), i64 1 }, align 32
// CHECK: @nonvirt = addrspace(200) global { i8 addrspace(200)*, i64 } { i8 addrspace(200)* addrspacecast (i8* bitcast (void ()* @_Z4funcv to i8*) to i8 addrspace(200)*), i64 1 }, align 32

// now the real thing
// FIXME: why is alignment only 8 here....

typedef int (A::* AMemberFuncPtr)();

AMemberFuncPtr global_null_func_ptr = nullptr;
int A::* global_data_ptr = &A::y;
AMemberFuncPtr global_nonvirt_func_ptr = &A::bar;
AMemberFuncPtr global_virt_func_ptr = &A::bar_virtual;
// CHECK: @global_null_func_ptr = addrspace(200) global { i8 addrspace(200)*, i64 } zeroinitializer, align 32
// CHECK: @global_data_ptr = addrspace(200) global i64 36, align 8
// CHECK: @global_nonvirt_func_ptr = addrspace(200) global { i8 addrspace(200)*, i64 } { i8 addrspace(200)* addrspacecast (i8* bitcast (i32 (%class.A addrspace(200)*)* @_ZN1A3barEv to i8*) to i8 addrspace(200)*), i64 0 }, align 32
// CHECK: @global_virt_func_ptr = addrspace(200) global { i8 addrspace(200)*, i64 } { i8 addrspace(200)* inttoptr (i64 32 to i8 addrspace(200)*), i64 1 }, align 32


int main() {
  // CHECK: %data_ptr = alloca i64, align 8
  // CHECK-NOT: %func_ptr = alloca { i64, i64 }, align 8
  // CHECK-NOT: %virtual_func_ptr = alloca { i64, i64 }, align 8
  A a;
  // FIXME: alignment is wrong
  int A::* null_data_ptr = nullptr;
  // CHECK: store i64 -1, i64 addrspace(200)* %null_data_ptr, align 8
  int A::* data_ptr = &A::x;
  // CHECK: store i64 32, i64 addrspace(200)* %data_ptr, align 8
  int A::* data_ptr_2 = &A::y;
  // CHECK: store i64 36, i64 addrspace(200)* %data_ptr_2, align 8

  AMemberFuncPtr null_func_ptr = nullptr;
  // CHECK:   store { i8 addrspace(200)*, i64 } zeroinitializer, { i8 addrspace(200)*, i64 } addrspace(200)* %null_func_ptr, align 32

  AMemberFuncPtr func_ptr = &A::foo;
  // This IR is pretty horrible, maybe we can create something nicer
  // CHECK: [[PCC:%.*]] = call i8 addrspace(200)* @llvm.cheri.pcc.get()
  // CHECK: [[NONVIRT_PTR:%.*]] = call i8 addrspace(200)* @llvm.cheri.cap.offset.set(i8 addrspace(200)* [[PCC]], i64 ptrtoint (i32 (%class.A addrspace(200)*)* @_ZN1A3fooEv to i64))
  // CHECK: [[TMP:%.*]] = getelementptr inbounds { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %memptr_tmp, i32 0, i32 0
  // CHECK: store i8 addrspace(200)* [[NONVIRT_PTR]], i8 addrspace(200)* addrspace(200)* [[TMP]], align 32
  // CHECK: [[TMP:%.*]] = getelementptr inbounds { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %memptr_tmp, i32 0, i32 1
  // CHECK: store i64 0, i64 addrspace(200)* [[TMP]], align 32
  // CHECK: [[TMP:%.*]] = load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %memptr_tmp, align 32
  // CHECK: store { i8 addrspace(200)*, i64 } [[TMP]], { i8 addrspace(200)*, i64 } addrspace(200)* %func_ptr, align 32

  AMemberFuncPtr func_ptr_2 = &A::bar;
  // CHECK: [[PCC:%.*]] = call i8 addrspace(200)* @llvm.cheri.pcc.get()
  // CHECK: [[NONVIRT_PTR:%.*]] = call i8 addrspace(200)* @llvm.cheri.cap.offset.set(i8 addrspace(200)* [[PCC]], i64 ptrtoint (i32 (%class.A addrspace(200)*)* @_ZN1A3barEv to i64))
  // CHECK: [[TMP:%.*]] = getelementptr inbounds { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %memptr_tmp1, i32 0, i32 0
  // CHECK: store i8 addrspace(200)* [[NONVIRT_PTR]], i8 addrspace(200)* addrspace(200)* [[TMP]], align 32
  // CHECK: [[TMP:%.*]] = getelementptr inbounds { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %memptr_tmp1, i32 0, i32 1
  // CHECK: store i64 0, i64 addrspace(200)* [[TMP]], align 32
  // CHECK: [[TMP:%.*]] = load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %memptr_tmp1, align 32
  // CHECK: store { i8 addrspace(200)*, i64 } [[TMP]], { i8 addrspace(200)*, i64 } addrspace(200)* %func_ptr_2, align 32
  

AMemberFuncPtr virtual_func_ptr = &A::foo_virtual;
  // CHECK: store { i8 addrspace(200)*, i64 } { i8 addrspace(200)* null, i64 1 }, { i8 addrspace(200)*, i64 } addrspace(200)* %virtual_func_ptr, align 32
   AMemberFuncPtr virtual_func_ptr_2 = &A::bar_virtual;
  // CHECK: store { i8 addrspace(200)*, i64 } { i8 addrspace(200)* inttoptr (i64 32 to i8 addrspace(200)*), i64 1 }, { i8 addrspace(200)*, i64 } addrspace(200)* %virtual_func_ptr_2, align 32

  // return a.*data_ptr + (a.*func_ptr)() + (a.*virtual_func_ptr)();
  // return null_func_ptr == nullptr;
  return a.*data_ptr;
}

bool data_ptr_is_nonnull(int A::* ptr) {
  return static_cast<bool>(ptr);
  // CHECK: define zeroext i1 @_Z19data_ptr_is_nonnullM1Ai(i64 %ptr)
  // CHECK: %0 = load i64, i64 addrspace(200)* %ptr.addr, align 8
  // CHECK: %memptr.tobool = icmp ne i64 %0, -1
  // CHECK: ret i1 %memptr.tobool
}

bool data_ptr_is_null(int A::* ptr) {
  // CHECK: define zeroext i1 @_Z16data_ptr_is_nullM1Ai(i64 %ptr)
  // CHECK: %0 = load i64, i64 addrspace(200)* %ptr.addr, align 8
  // CHECK: %memptr.tobool = icmp ne i64 %0, -1
  // CHECK: %lnot = xor i1 %memptr.tobool, true
  // CHECK: ret i1 %lnot
  return !ptr;
}

bool data_ptr_equal(int A::* ptr1, int A::* ptr2) {
  return ptr1 == ptr2;
  // CHECK: define zeroext i1 @_Z14data_ptr_equalM1AiS0_(i64 %ptr1, i64 %ptr2)
  // CHECK: %0 = load i64, i64 addrspace(200)* %ptr1.addr, align 8
  // CHECK: %1 = load i64, i64 addrspace(200)* %ptr2.addr, align 8
  // CHECK: %2 = icmp eq i64 %0, %1
  // CHECK: ret i1 %2
}

bool data_ptr_not_equal(int A::* ptr1, int A::* ptr2) {
  return ptr1 != ptr2;
  // CHECK: define zeroext i1 @_Z18data_ptr_not_equalM1AiS0_(i64 %ptr1, i64 %ptr2)
  // CHECK: %0 = load i64, i64 addrspace(200)* %ptr1.addr, align 8
  // CHECK: %1 = load i64, i64 addrspace(200)* %ptr2.addr, align 8
  // CHECK: %2 = icmp ne i64 %0, %1
  // CHECK: ret i1 %2
}

int data_ptr_dereferece(A* a, int A::* ptr) {
  return a->*ptr;
  // CHECK: define i32 @_Z19data_ptr_derefereceP1AMS_i(%class.A addrspace(200)* %a, i64 %ptr)
  // CHECK: %0 = load %class.A addrspace(200)*, %class.A addrspace(200)* addrspace(200)* %a.addr, align 32
  // CHECK: %1 = load i64, i64 addrspace(200)* %ptr.addr, align 8
  // CHECK: %2 = bitcast %class.A addrspace(200)* %0 to i8 addrspace(200)*
  // CHECK: %memptr.offset = getelementptr inbounds i8, i8 addrspace(200)* %2, i64 %1
  // CHECK: %3 = bitcast i8 addrspace(200)* %memptr.offset to i32 addrspace(200)*
  // CHECK: %4 = load i32, i32 addrspace(200)* %3, align 4
  // CHECK: ret i32 %4
}

// TODO: this could be simplified to test the tag bit of the address instead
// of checking the low bit of the adjustment

bool func_ptr_is_nonnull(AMemberFuncPtr ptr) {
  return static_cast<bool>(ptr);
  // CHECK: zeroext i1 @_Z19func_ptr_is_nonnullM1AFivE(i8 addrspace(200)* inreg %ptr.coerce0, i64 inreg %ptr.coerce1)
  // CHECK: %memptr.ptr = extractvalue { i8 addrspace(200)*, i64 } %2, 0
  // CHECK: %memptr.tobool = icmp ne i8 addrspace(200)* %memptr.ptr, null
  // CHECK: %memptr.adj = extractvalue { i8 addrspace(200)*, i64 } %2, 1
  // CHECK: %memptr.virtualbit = and i64 %memptr.adj, 1
  // CHECK: %memptr.isvirtual = icmp ne i64 %memptr.virtualbit, 0
  // CHECK: %memptr.isnonnull = or i1 %memptr.tobool, %memptr.isvirtual
  // CHECK: ret i1 %memptr.isnonnull
}

bool func_ptr_is_null(AMemberFuncPtr ptr) {
  return !ptr;
  // CHECK: define zeroext i1 @_Z16func_ptr_is_nullM1AFivE(i8 addrspace(200)* inreg %ptr.coerce0, i64 inreg %ptr.coerce1)
  // CHECK: %memptr.ptr = extractvalue { i8 addrspace(200)*, i64 } %2, 0
  // CHECK: %memptr.tobool = icmp ne i8 addrspace(200)* %memptr.ptr, null
  // CHECK: %memptr.adj = extractvalue { i8 addrspace(200)*, i64 } %2, 1
  // CHECK: %memptr.virtualbit = and i64 %memptr.adj, 1
  // CHECK: %memptr.isvirtual = icmp ne i64 %memptr.virtualbit, 0
  // CHECK: %memptr.isnonnull = or i1 %memptr.tobool, %memptr.isvirtual
  // CHECK: %lnot = xor i1 %memptr.isnonnull, true
  // CHECK: ret i1 %lnot
}

bool func_ptr_equal(AMemberFuncPtr ptr1, AMemberFuncPtr ptr2) {
  return ptr1 == ptr2;
  // CHECK: define zeroext i1 @_Z14func_ptr_equalM1AFivES1_(i8 addrspace(200)* inreg %ptr1.coerce0, i64 inreg %ptr1.coerce1, i8 addrspace(200)* inreg %ptr2.coerce0, i64 inreg %ptr2.coerce1)
  // CHECK: %lhs.memptr.ptr = extractvalue { i8 addrspace(200)*, i64 } %4, 0
  // CHECK: %rhs.memptr.ptr = extractvalue { i8 addrspace(200)*, i64 } %5, 0
  // CHECK: %cmp.ptr = icmp eq i8 addrspace(200)* %lhs.memptr.ptr, %rhs.memptr.ptr
  // CHECK: %cmp.ptr.null = icmp eq i8 addrspace(200)* %lhs.memptr.ptr, null
  // CHECK: %lhs.memptr.adj = extractvalue { i8 addrspace(200)*, i64 } %4, 1
  // CHECK: %rhs.memptr.adj = extractvalue { i8 addrspace(200)*, i64 } %5, 1
  // CHECK: %cmp.adj = icmp eq i64 %lhs.memptr.adj, %rhs.memptr.adj
  // CHECK: %or.adj = or i64 %lhs.memptr.adj, %rhs.memptr.adj
  // CHECK: %6 = and i64 %or.adj, 1
  // CHECK: %cmp.or.adj = icmp eq i64 %6, 0
  // CHECK: %7 = and i1 %cmp.ptr.null, %cmp.or.adj
  // CHECK: %8 = or i1 %7, %cmp.adj
  // CHECK: %memptr.eq = and i1 %cmp.ptr, %8
  // CHECK: ret i1 %memptr.eq
}

bool func_ptr_not_equal(AMemberFuncPtr ptr1, AMemberFuncPtr ptr2) {
  return ptr1 != ptr2;
  // CHECK: zeroext i1 @_Z18func_ptr_not_equalM1AFivES1_(i8 addrspace(200)* inreg %ptr1.coerce0, i64 inreg %ptr1.coerce1, i8 addrspace(200)* inreg %ptr2.coerce0, i64 inreg %ptr2.coerce1)
  // CHECK: %lhs.memptr.ptr = extractvalue { i8 addrspace(200)*, i64 } %4, 0
  // CHECK: %rhs.memptr.ptr = extractvalue { i8 addrspace(200)*, i64 } %5, 0
  // CHECK: %cmp.ptr = icmp ne i8 addrspace(200)* %lhs.memptr.ptr, %rhs.memptr.ptr
  // CHECK: %cmp.ptr.null = icmp ne i8 addrspace(200)* %lhs.memptr.ptr, null
  // CHECK: %lhs.memptr.adj = extractvalue { i8 addrspace(200)*, i64 } %4, 1
  // CHECK: %rhs.memptr.adj = extractvalue { i8 addrspace(200)*, i64 } %5, 1
  // CHECK: %cmp.adj = icmp ne i64 %lhs.memptr.adj, %rhs.memptr.adj
  // CHECK: %or.adj = or i64 %lhs.memptr.adj, %rhs.memptr.adj
  // CHECK: %6 = and i64 %or.adj, 1
  // CHECK: %cmp.or.adj = icmp ne i64 %6, 0
  // CHECK: %7 = or i1 %cmp.ptr.null, %cmp.or.adj
  // CHECK: %8 = and i1 %7, %cmp.adj
  // CHECK: %memptr.ne = or i1 %cmp.ptr, %8
  // CHECK: ret i1 %memptr.ne
}

int func_ptr_dereference(A* a, AMemberFuncPtr ptr) {
  return (a->*ptr)();
  // CHECK: define i32 @_Z20func_ptr_dereferenceP1AMS_FivE(%class.A addrspace(200)* %a, i8 addrspace(200)* inreg %ptr.coerce0, i64 inreg %ptr.coerce1)
  // CHECK: %2 = load %class.A addrspace(200)*, %class.A addrspace(200)* addrspace(200)* %a.addr, align 32
  // CHECK: %3 = load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %ptr.addr, align 32
  // CHECK: %memptr.adj = extractvalue { i8 addrspace(200)*, i64 } %3, 1
  // CHECK: %memptr.adj.shifted = ashr i64 %memptr.adj, 1
  // CHECK: %this.not.adjusted = bitcast %class.A addrspace(200)* %2 to i8 addrspace(200)*
  // CHECK: %memptr.vtable.addr = getelementptr inbounds i8, i8 addrspace(200)* %this.not.adjusted, i64 %memptr.adj.shifted
  // CHECK: %this.adjusted = bitcast i8 addrspace(200)* %memptr.vtable.addr to %class.A addrspace(200)*
  // CHECK: %memptr.ptr = extractvalue { i8 addrspace(200)*, i64 } %3, 0
  // CHECK: %4 = and i64 %memptr.adj, 1
  // CHECK: %memptr.isvirtual = icmp ne i64 %4, 0
  // CHECK: br i1 %memptr.isvirtual, label %memptr.virtual, label %memptr.nonvirtual

  // CHECK: memptr.virtual:                                   ; preds = %entry
  // CHECK: %5 = bitcast i8 addrspace(200)* %memptr.vtable.addr to i8 addrspace(200)* addrspace(200)*
  // CHECK: %vtable = load i8 addrspace(200)*, i8 addrspace(200)* addrspace(200)* %5, align 32
  // CHECK: %memptr.vtable.offset = ptrtoint i8 addrspace(200)* %memptr.ptr to i64
  // CHECK: %6 = getelementptr i8, i8 addrspace(200)* %vtable, i64 %memptr.vtable.offset
  // CHECK: %7 = bitcast i8 addrspace(200)* %6 to i32 (%class.A addrspace(200)*) addrspace(200)* addrspace(200)*
  // CHECK: %memptr.virtualfn = load i32 (%class.A addrspace(200)*) addrspace(200)*, i32 (%class.A addrspace(200)*) addrspace(200)* addrspace(200)* %7, align 32
  // CHECK: br label %memptr.end

  // CHECK: memptr.nonvirtual:                                ; preds = %entry
  // CHECK: %memptr.nonvirtualfn = bitcast i8 addrspace(200)* %memptr.ptr to i32 (%class.A addrspace(200)*) addrspace(200)*
  // CHECK: br label %memptr.end

  // CHECK: memptr.end:                                       ; preds = %memptr.nonvirtual, %memptr.virtual
  // CHECK: %8 = phi i32 (%class.A addrspace(200)*) addrspace(200)* [ %memptr.virtualfn, %memptr.virtual ], [ %memptr.nonvirtualfn, %memptr.nonvirtual ]
  // CHECK: %call = call i32 %8(%class.A addrspace(200)* %this.adjusted)
  // CHECK: ret i32 %call
  // N64: %memptr.adj.shifted = ashr i64 %ptr.coerce1, 1
  // N64: %this.not.adjusted = bitcast %class.A* %a to i8*
  // N64: %memptr.vtable.addr = getelementptr inbounds i8, i8* %this.not.adjusted, i64 %memptr.adj.shifted
  // N64: %this.adjusted = bitcast i8* %memptr.vtable.addr to %class.A*
  // N64: %0 = and i64 %ptr.coerce1, 1
  // N64: %memptr.isvirtual = icmp eq i64 %0, 0
  // N64: br i1 %memptr.isvirtual, label %memptr.nonvirtual, label %memptr.virtual

  // N64: memptr.virtual:
  // N64: %1 = bitcast i8* %memptr.vtable.addr to i8**
  // N64: %vtable = load i8*, i8** %1, align 8, !tbaa !6
  // N64: %2 = getelementptr i8, i8* %vtable, i64 %ptr.coerce0
  // N64: %3 = bitcast i8* %2 to i32 (%class.A*)**
  // N64: %memptr.virtualfn = load i32 (%class.A*)*, i32 (%class.A*)** %3, align 8
  // N64: br label %memptr.end

  // N64: memptr.nonvirtual:
  // N64: %memptr.nonvirtualfn = inttoptr i64 %ptr.coerce0 to i32 (%class.A*)*
  // N64: br label %memptr.end

  // N64: memptr.end:
  // N64: %4 = phi i32 (%class.A*)* [ %memptr.virtualfn, %memptr.virtual ], [ %memptr.nonvirtualfn, %memptr.nonvirtual ]
  // N64: %call = tail call i32 %4(%class.A* %this.adjusted)
  // N64: ret i32 %call

  // N64 ASM on entry: $4 = A* a, $5 = memptr.ptr, $6 = memptr.adj
  // shift right by 1 to load adj from memptr.adj into $1
  // N64-ASM: dsra    [[ADJ:\$1]], $6, 1
  // $2 = isvirtual
  // N64-ASM: andi    $2, $6, 1
  // N64-ASM: beqz    $2, .LBB12_2
  // branch delay: add adj to $4 to get this.adjusted
  // N64-ASM: daddu   [[THIS_ADJUSTED:\$4]], $4, [[ADJ]]
  // load vtable into $1:
  // N64-ASM: ld      [[VTABLE:\$1]], 0([[THIS_ADJUSTED]])
  // add memptr.ptr (cast to int) to the vtable to get the index:
  // N64-ASM: daddu   [[VTABLE]], [[VTABLE]], $5
  // load the function pointer into $5 (which would already contain it in the non-virtual case)
  // N64-ASM: ld      $5, 0([[VTABLE]])
  // N64-ASM: move    $25, $5
  // N64-ASM: jalr    $25

}

// Check using Member pointers as return values an parameters
AMemberFuncPtr return_func_ptr() {
  // CHECK: define void @_Z15return_func_ptrv({ i8 addrspace(200)*, i64 } addrspace(200)* noalias sret %agg.result)
  // CHECK: store { i8 addrspace(200)*, i64 } { i8 addrspace(200)* inttoptr (i64 32 to i8 addrspace(200)*), i64 1 }, { i8 addrspace(200)*, i64 } addrspace(200)* %retval, align 32
  // CHECK: %0 = load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %retval, align 32
  // CHECK: store { i8 addrspace(200)*, i64 } %0, { i8 addrspace(200)*, i64 } addrspace(200)* %agg.result, align 32
  // CHECK: ret void
  return &A::bar_virtual;
}

void take_func_ptr(AMemberFuncPtr ptr) {
  // CHECK: define void @_Z13take_func_ptrM1AFivE(i8 addrspace(200)* inreg %ptr.coerce0, i64 inreg %ptr.coerce1)
  // CHECK: ret void
}

AMemberFuncPtr passthrough_func_ptr(AMemberFuncPtr ptr) {
  // CHECK: define void @_Z20passthrough_func_ptrM1AFivE({ i8 addrspace(200)*, i64 } addrspace(200)* noalias sret %agg.result, i64, i8 addrspace(200)* inreg %ptr.coerce0, i64 inreg %ptr.coerce1)
  // CHECK: %1 = getelementptr inbounds { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %ptr, i32 0, i32 0
  // CHECK: store i8 addrspace(200)* %ptr.coerce0, i8 addrspace(200)* addrspace(200)* %1, align 32
  // CHECK: %2 = getelementptr inbounds { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %ptr, i32 0, i32 1
  // CHECK: store i64 %ptr.coerce1, i64 addrspace(200)* %2, align 32
  // CHECK: %ptr1 = load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %ptr, align 32
  // CHECK: store { i8 addrspace(200)*, i64 } %ptr1, { i8 addrspace(200)*, i64 } addrspace(200)* %ptr.addr, align 32
  // CHECK: %3 = load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %ptr.addr, align 32
  // CHECK: store { i8 addrspace(200)*, i64 } %3, { i8 addrspace(200)*, i64 } addrspace(200)* %retval, align 32
  // CHECK: %4 = load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* %retval, align 32
  // CHECK: store { i8 addrspace(200)*, i64 } %4, { i8 addrspace(200)*, i64 } addrspace(200)* %agg.result, align 32
  // CHECK: ret void
  return ptr;
}

// taken from temporaries.cpp
namespace PR7556 {
  struct A { ~A(); };
  struct B { int i; ~B(); };
  struct C { int C::*pm; ~C(); };
  // CHECK-LABEL: define void @_ZN6PR75563fooEv()
  void foo() {
    // CHECK: call void @_ZN6PR75561AD1Ev(%"struct.PR7556::A" addrspace(200)* %agg.tmp.ensured)
    A();

    // B() is initialized using memset:
    // CHECK: %0 = bitcast %"struct.PR7556::B" addrspace(200)* %agg.tmp.ensured1 to i8 addrspace(200)*
    // CHECK: call void @llvm.memset.p200i8.i64(i8 addrspace(200)* %0, i8 0, i64 4, i32 4, i1 false)
    // CHECK: call void @_ZN6PR75561BD1Ev(%"struct.PR7556::B" addrspace(200)* %agg.tmp.ensured1)
    B();
    // C can't be zero-initialized due to pointer to data member:
    // CHECK: %1 = bitcast %"struct.PR7556::C" addrspace(200)* %agg.tmp.ensured2 to i8 addrspace(200)*
    // CHECK: call void @llvm.memcpy.p200i8.p200i8.i64(i8 addrspace(200)* %1, i8 addrspace(200)* addrspacecast (i8* bitcast (%"struct.PR7556::C"* @0 to i8*) to i8 addrspace(200)*), i64 8, i32 8, i1 false)
    // CHECK: call void @_ZN6PR75561CD1Ev(%"struct.PR7556::C" addrspace(200)* %agg.tmp.ensured2)
    C();
    // CHECK: ret void
  }
}
