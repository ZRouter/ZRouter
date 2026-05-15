--- include/mruby/class.h.org	2026-04-20 17:43:06.000000000 +0900
+++ include/mruby/class.h	2026-04-26 06:57:56.175360000 +0900
@@ -100,10 +100,13 @@
 typedef int (mrb_mt_foreach_func)(mrb_state*,mrb_sym,mrb_method_t,void*);
 MRB_API void mrb_mt_foreach(mrb_state*, struct RClass*, mrb_mt_foreach_func*, void*);
 
-/* ROM method table types for static method registration */
+/* ROM method table types for static method registration.
+ * NOTE: `func` is kept as the first union member so that positional
+ * aggregate initialization in MRB_MT_ENTRY works without C99
+ * designated initializers (required for legacy C++ compilers). */
 union mrb_mt_ptr {
-  const struct RProc *proc;
   mrb_func_t func;
+  const struct RProc *proc;
 };
 
 /* entry combining function pointer, symbol key, and flags */
@@ -128,7 +131,7 @@
 
 /* ROM table entry: 3rd param is MRB_ARGS_*() optionally OR'd with MRB_MT_PRIVATE. */
 #define MRB_MT_ENTRY(fn, sym, flags) \
-  { { .func = (fn) }, (sym), (flags) | MRB_MT_FUNC }
+  { { (fn) }, (sym), (flags) | MRB_MT_FUNC }
 #define MRB_MT_ASPEC(flags) ((mrb_aspec)((flags) & 0xffffff))
 
 /* "removed" tombstone: MRB_MT_FUNC flag set with NULL function pointer.
