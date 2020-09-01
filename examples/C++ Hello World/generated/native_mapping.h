#ifndef SHARP_NATIVE_MAPPING_H
#define SHARP_NATIVE_MAPPING_H

#include "snb_api.h"

using namespace snb_api;

scope_begin(app)

    void __srt_global(object $instance);
    void print_message();
    var main(object args);
scope_end()

scope_begin(std)

    void __srt_global(object $instance);
    var_array snprintf(var& fmt, var& num, var& precision);
    void print(_int8_array& data);
    void print2(object str);
    void println(var_array& data);
    void println2(_int8_array& data);
    void println3(_int16_array& data);
    void println4(_int32_array& data);
    void println5(_int64_array& data);
    void println6(_uint8_array& data);
    void println7(_uint16_array& data);
    void println8(_uint32_array& data);
    void println9(_uint64_array& data);
    void println10(var& data);
    void println11();
    void println12(object e);
    object read_line();
    object read_passwd();
    var read_char();
    var read_raw_char();
    void println13(object str);
    void println14(object o);
    void print3(object o);
    void print4(var& data);
    void flush();
    var utc_mills_time();
    var nano_time();
    void exit();
    var sizeOf(object data);
    var sizeOf2(object data);
    void print_chars(_int8_array& str);
    void print_num(var& val);
    void print_char(_int8& val);
    void print_numbers(var_array& str);
    void print_numbers2(_int16_array& str);
    void print_numbers3(_int32_array& str);
    void print_numbers4(_int64_array& str);
    void print_numbers5(_uint8_array& str);
    void print_numbers6(_uint16_array& str);
    void print_numbers7(_uint32_array& str);
    void print_numbers8(_uint64_array& str);
    void print_chars2(_int16_array& str);
    var read_ch(var& hide);
    object runtime_error(object message);
    object runtime_error2();
    void todo(object message);
    void $03internal_static_init();
scope_end()

scope_begin(std, _enum_)

    void _enum_(object $instance, var& ord);
    void _enum_2(object $instance, object e);
    var op_$not_equals(object $instance, object e);
    var op_$not_equals2(object $instance, var& num);
    var op_$equals_equals(object $instance, object e);
    var op_$equals_equals2(object $instance, var& num);
    _int8_array get_name(object $instance);
    _int64 get_ordinal(object $instance);
    object to_string(object $instance);
    void _enum_3(object $instance);
scope_end()

scope_begin(std, throwable)

    void throwable(object $instance, _int8_array& message);
    void throwable2(object $instance, object message);
    object get_message(object $instance);
    void throwable3(object $instance);
    object get_stack_trace(object $instance);
scope_end()

scope_begin(std, exception)

    void exception(object $instance, _int8_array& message);
    void exception2(object $instance, object message);
    void exception3(object $instance);
scope_end()

scope_begin(std, illegal_argument_exception)

    void illegal_argument_exception(object $instance, _int8_array& message);
    void illegal_argument_exception2(object $instance, object message);
    void illegal_argument_exception3(object $instance);
scope_end()

scope_begin(std, illegal_state_exception)

    void illegal_state_exception(object $instance, _int8_array& message);
    void illegal_state_exception2(object $instance, object message);
    void illegal_state_exception3(object $instance);
scope_end()

scope_begin(std, out_of_bounds_exception)

    void out_of_bounds_exception(object $instance, _int8_array& message);
    void out_of_bounds_exception2(object $instance, object message);
    void out_of_bounds_exception3(object $instance);
scope_end()

scope_begin(std, io_exception)

    void io_exception(object $instance, _int8_array& message);
    void io_exception2(object $instance, object message);
    void io_exception3(object $instance);
scope_end()

scope_begin(std, invalid_operation_exception)

    void invalid_operation_exception(object $instance, _int8_array& message);
    void invalid_operation_exception2(object $instance, object message);
    void invalid_operation_exception3(object $instance);
scope_end()

scope_begin(std, nullptr_exception)

    void nullptr_exception(object $instance, _int8_array& message);
    void nullptr_exception2(object $instance, object message);
    void nullptr_exception3(object $instance);
scope_end()

scope_begin(std, runtime_exception)

    void runtime_exception(object $instance, _int8_array& message);
    void runtime_exception2(object $instance, object message);
    void runtime_exception3(object $instance);
scope_end()

scope_begin(std, stack_overflow_exception)

    void stack_overflow_exception(object $instance, _int8_array& message);
    void stack_overflow_exception2(object $instance, object message);
    void stack_overflow_exception3(object $instance);
scope_end()

scope_begin(std, thread_stack_exception)

    void thread_stack_exception(object $instance, _int8_array& message);
    void thread_stack_exception2(object $instance, object message);
    void thread_stack_exception3(object $instance);
scope_end()

scope_begin(std, class_cast_exception)

    void class_cast_exception(object $instance, _int8_array& message);
    void class_cast_exception2(object $instance, object message);
    void class_cast_exception3(object $instance);
scope_end()

scope_begin(std, out_of_memory_exception)

    void out_of_memory_exception(object $instance, _int8_array& message);
    void out_of_memory_exception2(object $instance, object message);
    void out_of_memory_exception3(object $instance);
scope_end()

scope_begin(std, not_implemented_error)

    void not_implemented_error(object $instance, _int8_array& message);
    void not_implemented_error2(object $instance, object message);
    void not_implemented_error3(object $instance);
scope_end()

scope_begin(std, unsatisfied_link_error)

    void unsatisfied_link_error(object $instance, _int8_array& message);
    void unsatisfied_link_error2(object $instance, object message);
    void unsatisfied_link_error3(object $instance);
scope_end()

scope_begin(std_io)

    void __srt_global(object $instance);
    void critical_section(var& block);
    void critical_section2(var& block, object lock_obj);
    void realloc(object data, var& size);
    void realloc2(var_array& data, var& size);
    void realloc3(_int8_array& data, var& size);
    void realloc4(_int16_array& data, var& size);
    void realloc5(_int32_array& data, var& size);
    void realloc6(_int64_array& data, var& size);
    void realloc7(_uint8_array& data, var& size);
    void realloc8(_uint16_array& data, var& size);
    void realloc9(_uint32_array& data, var& size);
    void realloc10(_uint64_array& data, var& size);
    void memcopy(object src, object dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy2(var_array& src, var_array& dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy3(_int8_array& src, _int8_array& dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy4(_int16_array& src, _int16_array& dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy5(_int32_array& src, _int32_array& dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy6(_int64_array& src, _int64_array& dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy7(_uint8_array& src, _uint8_array& dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy8(_uint16_array& src, _uint16_array& dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy9(_uint32_array& src, _uint32_array& dest, var& destStart, var& srcStart, var& srcEnd);
    void memcopy10(_uint64_array& src, _uint64_array& dest, var& destStart, var& srcStart, var& srcEnd);
    object copy(object src, var& srcStart, var& srcEnd);
    var_array copy2(var_array& src, var& srcStart, var& srcEnd);
    _int8_array copy3(_int8_array& src, var& srcStart, var& srcEnd);
    _int16_array copy4(_int16_array& src, var& srcStart, var& srcEnd);
    _int32_array copy5(_int32_array& src, var& srcStart, var& srcEnd);
    _uint8_array copy6(_uint8_array& src, var& srcStart, var& srcEnd);
    _uint16_array copy7(_uint16_array& src, var& srcStart, var& srcEnd);
    _uint32_array copy8(_uint32_array& src, var& srcStart, var& srcEnd);
    _uint64_array copy9(_uint64_array& src, var& srcStart, var& srcEnd);
    _int64_array copy10(_int64_array& src, var& srcStart, var& srcEnd);
    object invert(object src, var& start, var& len);
    var_array invert2(var_array& src, var& start, var& len);
    _int8_array invert3(_int8_array& src, var& start, var& len);
    _int16_array invert4(_int16_array& src, var& start, var& len);
    _int32_array invert5(_int32_array& src, var& start, var& len);
    _int64_array invert6(_int64_array& src, var& start, var& len);
    _uint8_array invert7(_uint8_array& src, var& start, var& len);
    _uint16_array invert8(_uint16_array& src, var& start, var& len);
    _uint32_array invert9(_uint32_array& src, var& start, var& len);
    _uint64_array invert10(_uint64_array& src, var& start, var& len);
    void _srt_thread_start();
    void $03internal_static_init();
scope_end()

scope_begin(std_io, file)

    void file(object $instance, object path);
    void file2(object $instance, _int8_array& path);
    object get_name(object $instance);
    object to_string(object $instance);
    void update_path(object $instance, object path);
    object get_parent_file(object $instance);
    object get_path(object $instance);
    object get_absolute_path(object $instance);
    var is_readable(object $instance);
    var is_writeable(object $instance);
    var exists(object $instance);
    var is_directory(object $instance);
    var is_file(object $instance);
    var is_hidden(object $instance);
    var last_modified(object $instance);
    var last_status_changed(object $instance);
    var last_accessed(object $instance);
    var length(object $instance);
    void create(object $instance);
    var _delete(object $instance);
    object list(object $instance);
    var mkdir(object $instance);
    var rename(object $instance, object dest);
    var set_last_modified(object $instance, var& time);
    var set_read_only(object $instance);
    var set_writable(object $instance, var& writable, var& owner_only);
    var set_writable2(object $instance, var& writable);
    var set_readable(object $instance, var& readable, var& owner_only);
    var set_readable2(object $instance, var& readable);
    var set_executable(object $instance, var& executable, var& owner_only);
    var set_executable2(object $instance, var& executable);
    var is_executable(object $instance);
    var get_total_space(object $instance);
    var get_free_space(object $instance);
    var get_usable_space(object $instance);
    object generate_temp_file(object prefix, object suffix, object dir);
    object create_temp_file(object prefix, object suffix, object directory);
    object create_temp_file2(object prefix, object suffix);
    object read_all_text(object $instance);
    var write(object $instance, object data);
    object get_file_system();
    void file3(object $instance);
    void $03internal_static_init();
scope_end()

scope_begin(std_io, file_system)

    var path_separator(object $instance);
    object normalize(object $instance, object path);
    object resolve(object $instance, object f);
    var get_attributes(object $instance, object f);
    var check_access(object $instance, object f, var& access);
    var set_permission(object $instance, object f, var& access, var& enable, var& owneronly);
    var last_modified_time(object $instance, object f, var& stat_id);
    var get_length(object $instance, object f);
    void create_file(object $instance, object pathname);
    var delete_directory(object $instance, object f);
    var _delete(object $instance, object f);
    object list(object $instance, object f);
    var create_directory(object $instance, object f);
    var rename(object $instance, object f1, object f2);
    object read(object f);
    var write(object f, object data);
    var set_last_modified_time(object $instance, object f, var& time);
    var set_read_only(object $instance, object f);
    var get_space_available(var& request);
    void file_system(object $instance);
scope_end()

scope_begin(std_io_task)

    void __srt_global(object $instance);
    void start_scheduler();
    void calculate_max_threads();
    void set_thread_max(var& max);
    var get_thread_max();
    var scheduler_main(object args);
    object get_host(var& long_running);
    void execute_synchronous(object scheduled_job);
    var execute_job(object scheduled_job);
    void finish();
    void $03internal_static_init();
    var anon_func$3196(object it, object it2);
    var anon_func$3197(object it, object it2);
    var anon_func$3198(object it, object it2);
    var anon_func$3199(object t);
scope_end()

scope_begin(std_io_task, cancellation_exception)

    void cancellation_exception(object $instance, _int8_array& message);
    void cancellation_exception2(object $instance, object message);
    void cancellation_exception3(object $instance);
scope_end()

scope_begin(std_io_task, job)

    void job(object $instance);
    void job2(object $instance, object name);
    var execute_async(object $instance);
    void cancel(object $instance);
    void cancel_future(object $instance);
    void join(object $instance);
    object get_state(object $instance);
    object get_name(object $instance);
    var get_id(object $instance);
    void op_$equals(object $instance, var& block);
    void op_$equals2(object $instance, var& block);
    void op_$equals3(object $instance, var& block);
    void op_$equals4(object $instance, var& block);
    var job_main(object args);
    void execute(object $instance);
    var is_finished(object $instance);
    var is_active(object $instance);
    void update_timeout(object $instance, var& tm);
    void update_interval(object $instance, var& tm, object interval);
    void reschedule(object $instance);
    void $03internal_static_init();
scope_end()

scope_begin(std_io_task, job_builder)

    void job_builder(object $instance);
    void job_builder2(object $instance, object scheduled_job, object master);
    object with_timeout(object $instance, var& mills_time);
    object schedule(object $instance, var& future);
    object recurring(object $instance, var& tm, object interval);
    object with_name(object $instance, object job_name);
    object with_args(object $instance, object args);
    object block(object $instance);
    object long_term(object $instance);
    object op_$equals(object $instance, var& main);
    object op_$equals2(object $instance, var& main);
    object op_$equals3(object $instance, var& main);
    object op_$equals4(object $instance, var& main);
    object get_job(object $instance);
scope_end()

scope_begin(std_io_task, job_controller)

    void job_controller(object $instance);
scope_end()

scope_begin(std_io_task, job_master)

    void job_master(object $instance);
    object op_$equals(object $instance, var& main);
    object create_job(object $instance);
    object create_job2(object $instance, object name);
    void start_job(object $instance, object scheduled_job);
    object get_host_controller(object $instance, object host);
    void cleanup_job(object $instance, object scheduled_job);
scope_end()

scope_begin(std_io_task, task)

    object with_timeout(var& mills_time);
    object schedule(var& future);
    object with_name(object job_name);
    object with_args(object args);
    object block();
    object long_term();
    object builder();
    void task(object $instance);
scope_end()

scope_begin(std_io, thread)

    void thread(object $instance);
    void thread2(object $instance, object name, var& main);
    void thread3(object $instance, object name, var& daemon, var& main);
    void thread4(object $instance, object name, var& main, var& stack_size);
    void thread5(object $instance, object name, var& daemon, var& main, var& stack_size);
    void thread6(object $instance, object name, var& main, var& stack_size, object group);
    void thread7(object $instance, object name, var& main, object group);
    void thread8(object $instance, object name, var& daemon, var& main, var& stack_size, object group);
    object start(object $instance, object args);
    object start2(object $instance);
    object suspend(object $instance);
    object unsuspend(object $instance);
    object unsuspend_and_wait(object $instance);
    object join(object $instance);
    object interrupt(object $instance);
    object current();
    void sleep(var& mills_time);
    void wait();
    void wait2(var& mills_time);
    void exit(var& code);
    object hash(object $instance);
    void yield();
    void set_exception_handler(object $instance, object error_handler);
    object set_priority(object $instance, object priority);
    void remove_thread(object $instance, object t);
    var get_id(object $instance);
    var get_native_handle(object $instance);
    var get_stack_size(object $instance);
    object get_name(object $instance);
    var is_exited(object $instance);
    var is_daemon(object $instance);
    var is_started(object $instance);
    var is_suspended(object $instance);
    var set_main(object $instance, var& main);
    var is_active(object $instance);
    var get_exit_code(object $instance);
    object get_exception_handler(object $instance);
    var op_$equals_equals(object $instance, object t);
scope_end()

scope_begin(std_io, thread_group)

    void thread_group(object $instance, object name);
    void thread_group2(object $instance, object name, object priority);
    object at(object $instance, var& id);
    void add(object $instance, object t);
    var size(object $instance);
    object get_name(object $instance);
    object get_priority(object $instance);
    var remove(object $instance, var& id);
    object get_elements(object $instance);
    object find(object $instance, var& find_func);
    void kill_all(object $instance);
    void thread_group3(object $instance);
    void $03internal_static_init();
scope_end()

scope_begin(std_io, thread_result)

    object at(var& index);
    void thread_result(object $instance);
    void $03internal_static_init();
scope_end()

scope_begin(std_io, uncaught_exception_handler)

    void uncaught_exception(object $instance, object err);
    void uncaught_exception_handler(object $instance);
scope_end()

scope_begin(std_io, unix_fs)

    var path_separator(object $instance);
    object normalize(object $instance, object pathname);
    var get_attributes(object $instance, object f);
    var check_access(object $instance, object f, var& access);
    void unix_fs(object $instance);
scope_end()

scope_begin(std_io, win_fs)

    var path_separator(object $instance);
    object normalize(object $instance, object pathname);
    var get_attributes(object $instance, object f);
    var check_access(object $instance, object f, var& access);
    void win_fs(object $instance);
scope_end()

scope_begin(std_math)

    void __srt_global(object $instance);
    var is_nan(var& v);
    var is_infinite(var& v);
    var sin(var& num);
    var cos(var& num);
    var tan(var& num);
    var sinh(var& num);
    var cosh(var& num);
    var tanh(var& num);
    var asin(var& num);
    var acos(var& num);
    var atan(var& num);
    var atan2(var& num1, var& num2);
    var exp(var& num);
    var log(var& num);
    var log10(var& num);
    var pow(var& num, var& exp);
    var sqrt(var& num);
    var ceil(var& num);
    var floor(var& num);
    var abs(var& num);
    var fabs(var& num);
    var ldexp(var& num, var& num2);
    var fmod(var& num, var& num2);
    var round(var& num);
    var math_proc(var& num, var& proc);
    var math_proc2(var& num, var& num2, var& proc);
scope_end()

scope_begin(std, _nil_)

    object to_string(object $instance);
    void _nil_(object $instance);
    void set_instance(object value);
    void $03internal_static_init();
scope_end()

scope_begin(std, _object_)

    object to_string(object $instance);
    void wait(object $instance);
    void notify(object $instance);
    void notify2(object $instance, var& mills);
    object hash(object $instance);
    object guid(object $instance);
    void _object_(object $instance);
scope_end()

scope_begin(platform)

    void __srt_global(object $instance);
    void load_library(object name);
    var free_library(object name);
    var link(object name, object library);
scope_end()

scope_begin(platform, build)

    var is_windows();
    var is_linux();
    void build(object $instance);
scope_end()

scope_begin(platform_kernel)

    void __srt_global(object $instance);
scope_end()

scope_begin(platform_kernel, platform)

    void tls_init();
    void static_init();
    void exit(var& value);
    _int8_array get_lib_path(object name);
    void load_library(object name);
    var free_library(object name);
    var link_func(object name, object library);
    var srt_init(object args);
    void platform(object $instance);
scope_end()

scope_begin(platform_kernel, stack_state)

    void stack_state(object $instance);
scope_end()

scope_begin(platform_kernel, vm)

    object get_frame_info();
    object get_stack_trace(object info);
    var thread_create(var& daemon);
    object suspend(object t);
    object unsuspend(object t, var& wait);
    object thread_join(object t);
    object kill(object t);
    object current_thread();
    void exit_thread(object t, var& code);
    void thread_yield();
    object thread_start(object t, object args);
    object set_thread_priority(object t, var& priority);
    void set_thread(object t);
    object thread_args();
    var cores();
    void vm(object $instance);
scope_end()

scope_begin(std, ulong)

    void ulong(object $instance, object i);
    void ulong2(object $instance, object i);
    void ulong3(object $instance, object i);
    void ulong4(object $instance, object i);
    void ulong5(object $instance, object i);
    void ulong6(object $instance, object i);
    void ulong7(object $instance, object i);
    void ulong8(object $instance, object i);
    void ulong9(object $instance, object i);
    void ulong10(object $instance, object i);
    void ulong11(object $instance, var& i);
    _uint64 op_$not(object $instance);
    _uint64 op_$plus_plus(object $instance);
    _uint64 op_$plus_plus2(object $instance, var& num);
    _uint64 op_$minus_minus(object $instance);
    _uint64 op_$minus_minus2(object $instance, var& num);
    _uint64 op_$plus_equal(object $instance, object num);
    _uint64 op_$plus_equal2(object $instance, object num);
    _uint64 op_$plus_equal3(object $instance, object num);
    _uint64 op_$plus_equal4(object $instance, object num);
    _uint64 op_$plus_equal5(object $instance, object num);
    _uint64 op_$plus_equal6(object $instance, object num);
    _uint64 op_$plus_equal7(object $instance, object num);
    _uint64 op_$plus_equal8(object $instance, object num);
    _uint64 op_$plus_equal9(object $instance, object num);
    _uint64 op_$plus_equal10(object $instance, object num);
    _uint64 op_$plus_equal11(object $instance, var& num);
    _uint64 op_$minus_equal(object $instance, object num);
    _uint64 op_$minus_equal2(object $instance, object num);
    _uint64 op_$minus_equal3(object $instance, object num);
    _uint64 op_$minus_equal4(object $instance, object num);
    _uint64 op_$minus_equal5(object $instance, object num);
    _uint64 op_$minus_equal6(object $instance, object num);
    _uint64 op_$minus_equal7(object $instance, object num);
    _uint64 op_$minus_equal8(object $instance, object num);
    _uint64 op_$minus_equal9(object $instance, object num);
    _uint64 op_$minus_equal10(object $instance, object num);
    _uint64 op_$minus_equal11(object $instance, var& num);
    _uint64 op_$mult_equal(object $instance, object num);
    _uint64 op_$mult_equal2(object $instance, object num);
    _uint64 op_$mult_equal3(object $instance, object num);
    _uint64 op_$mult_equal4(object $instance, object num);
    _uint64 op_$mult_equal5(object $instance, object num);
    _uint64 op_$mult_equal6(object $instance, object num);
    _uint64 op_$mult_equal7(object $instance, object num);
    _uint64 op_$mult_equal8(object $instance, object num);
    _uint64 op_$mult_equal9(object $instance, object num);
    _uint64 op_$mult_equal10(object $instance, object num);
    _uint64 op_$mult_equal11(object $instance, var& num);
    _uint64 op_$div_equal(object $instance, object num);
    _uint64 op_$div_equal2(object $instance, object num);
    _uint64 op_$div_equal3(object $instance, object num);
    _uint64 op_$div_equal4(object $instance, object num);
    _uint64 op_$div_equal5(object $instance, object num);
    _uint64 op_$div_equal6(object $instance, object num);
    _uint64 op_$div_equal7(object $instance, object num);
    _uint64 op_$div_equal8(object $instance, object num);
    _uint64 op_$div_equal9(object $instance, object num);
    _uint64 op_$div_equal10(object $instance, object num);
    _uint64 op_$div_equal11(object $instance, var& num);
    _uint64 op_$mod_equal(object $instance, object num);
    _uint64 op_$mod_equal2(object $instance, object num);
    _uint64 op_$mod_equal3(object $instance, object num);
    _uint64 op_$mod_equal4(object $instance, object num);
    _uint64 op_$mod_equal5(object $instance, object num);
    _uint64 op_$mod_equal6(object $instance, object num);
    _uint64 op_$mod_equal7(object $instance, object num);
    _uint64 op_$mod_equal8(object $instance, object num);
    _uint64 op_$mod_equal9(object $instance, object num);
    _uint64 op_$mod_equal10(object $instance, object num);
    _uint64 op_$mod_equal11(object $instance, var& num);
    _uint64 op_$and_equal(object $instance, object num);
    _uint64 op_$and_equal2(object $instance, object num);
    _uint64 op_$and_equal3(object $instance, object num);
    _uint64 op_$and_equal4(object $instance, object num);
    _uint64 op_$and_equal5(object $instance, object num);
    _uint64 op_$and_equal6(object $instance, object num);
    _uint64 op_$and_equal7(object $instance, object num);
    _uint64 op_$and_equal8(object $instance, object num);
    _uint64 op_$and_equal9(object $instance, object num);
    _uint64 op_$and_equal10(object $instance, object num);
    _uint64 op_$and_equal11(object $instance, var& num);
    _uint64 op_$or_equal(object $instance, object num);
    _uint64 op_$or_equal2(object $instance, object num);
    _uint64 op_$or_equal3(object $instance, object num);
    _uint64 op_$or_equal4(object $instance, object num);
    _uint64 op_$or_equal5(object $instance, object num);
    _uint64 op_$or_equal6(object $instance, object num);
    _uint64 op_$or_equal7(object $instance, object num);
    _uint64 op_$or_equal8(object $instance, object num);
    _uint64 op_$or_equal9(object $instance, object num);
    _uint64 op_$or_equal10(object $instance, object num);
    _uint64 op_$or_equal11(object $instance, var& num);
    void ulong12(object $instance);
scope_end()

scope_begin(std, _long)

    void _long(object $instance, object i);
    void _long2(object $instance, object i);
    void _long3(object $instance, object i);
    void _long4(object $instance, object i);
    void _long5(object $instance, object i);
    void _long6(object $instance, object i);
    void _long7(object $instance, object i);
    void _long8(object $instance, object i);
    void _long9(object $instance, object i);
    void _long10(object $instance, object i);
    void _long11(object $instance, var& i);
    _int64 op_$not(object $instance);
    _int64 op_$plus_plus(object $instance);
    _int64 op_$plus_plus2(object $instance, var& num);
    _int64 op_$minus_minus(object $instance);
    _int64 op_$minus_minus2(object $instance, var& num);
    _int64 op_$plus_equal(object $instance, object num);
    _int64 op_$plus_equal2(object $instance, object num);
    _int64 op_$plus_equal3(object $instance, object num);
    _int64 op_$plus_equal4(object $instance, object num);
    _int64 op_$plus_equal5(object $instance, object num);
    _int64 op_$plus_equal6(object $instance, object num);
    _int64 op_$plus_equal7(object $instance, object num);
    _int64 op_$plus_equal8(object $instance, object num);
    _int64 op_$plus_equal9(object $instance, object num);
    _int64 op_$plus_equal10(object $instance, object num);
    _int64 op_$plus_equal11(object $instance, var& num);
    _int64 op_$minus_equal(object $instance, object num);
    _int64 op_$minus_equal2(object $instance, object num);
    _int64 op_$minus_equal3(object $instance, object num);
    _int64 op_$minus_equal4(object $instance, object num);
    _int64 op_$minus_equal5(object $instance, object num);
    _int64 op_$minus_equal6(object $instance, object num);
    _int64 op_$minus_equal7(object $instance, object num);
    _int64 op_$minus_equal8(object $instance, object num);
    _int64 op_$minus_equal9(object $instance, object num);
    _int64 op_$minus_equal10(object $instance, object num);
    _int64 op_$minus_equal11(object $instance, var& num);
    _int64 op_$mult_equal(object $instance, object num);
    _int64 op_$mult_equal2(object $instance, object num);
    _int64 op_$mult_equal3(object $instance, object num);
    _int64 op_$mult_equal4(object $instance, object num);
    _int64 op_$mult_equal5(object $instance, object num);
    _int64 op_$mult_equal6(object $instance, object num);
    _int64 op_$mult_equal7(object $instance, object num);
    _int64 op_$mult_equal8(object $instance, object num);
    _int64 op_$mult_equal9(object $instance, object num);
    _int64 op_$mult_equal10(object $instance, object num);
    _int64 op_$mult_equal11(object $instance, var& num);
    _int64 op_$div_equal(object $instance, object num);
    _int64 op_$div_equal2(object $instance, object num);
    _int64 op_$div_equal3(object $instance, object num);
    _int64 op_$div_equal4(object $instance, object num);
    _int64 op_$div_equal5(object $instance, object num);
    _int64 op_$div_equal6(object $instance, object num);
    _int64 op_$div_equal7(object $instance, object num);
    _int64 op_$div_equal8(object $instance, object num);
    _int64 op_$div_equal9(object $instance, object num);
    _int64 op_$div_equal10(object $instance, object num);
    _int64 op_$div_equal11(object $instance, var& num);
    _int64 op_$mod_equal(object $instance, object num);
    _int64 op_$mod_equal2(object $instance, object num);
    _int64 op_$mod_equal3(object $instance, object num);
    _int64 op_$mod_equal4(object $instance, object num);
    _int64 op_$mod_equal5(object $instance, object num);
    _int64 op_$mod_equal6(object $instance, object num);
    _int64 op_$mod_equal7(object $instance, object num);
    _int64 op_$mod_equal8(object $instance, object num);
    _int64 op_$mod_equal9(object $instance, object num);
    _int64 op_$mod_equal10(object $instance, object num);
    _int64 op_$mod_equal11(object $instance, var& num);
    _int64 op_$and_equal(object $instance, object num);
    _int64 op_$and_equal2(object $instance, object num);
    _int64 op_$and_equal3(object $instance, object num);
    _int64 op_$and_equal4(object $instance, object num);
    _int64 op_$and_equal5(object $instance, object num);
    _int64 op_$and_equal6(object $instance, object num);
    _int64 op_$and_equal7(object $instance, object num);
    _int64 op_$and_equal8(object $instance, object num);
    _int64 op_$and_equal9(object $instance, object num);
    _int64 op_$and_equal10(object $instance, object num);
    _int64 op_$and_equal11(object $instance, var& num);
    _int64 op_$or_equal(object $instance, object num);
    _int64 op_$or_equal2(object $instance, object num);
    _int64 op_$or_equal3(object $instance, object num);
    _int64 op_$or_equal4(object $instance, object num);
    _int64 op_$or_equal5(object $instance, object num);
    _int64 op_$or_equal6(object $instance, object num);
    _int64 op_$or_equal7(object $instance, object num);
    _int64 op_$or_equal8(object $instance, object num);
    _int64 op_$or_equal9(object $instance, object num);
    _int64 op_$or_equal10(object $instance, object num);
    _int64 op_$or_equal11(object $instance, var& num);
    void _long12(object $instance);
scope_end()

scope_begin(std, uint)

    void uint(object $instance, object i);
    void uint2(object $instance, object i);
    void uint3(object $instance, object i);
    void uint4(object $instance, object i);
    void uint5(object $instance, object i);
    void uint6(object $instance, object i);
    void uint7(object $instance, object i);
    void uint8(object $instance, object i);
    void uint9(object $instance, object i);
    void uint10(object $instance, object i);
    void uint11(object $instance, var& i);
    _uint32 op_$not(object $instance);
    _uint32 op_$plus_plus(object $instance);
    _uint32 op_$plus_plus2(object $instance, var& num);
    _uint32 op_$minus_minus(object $instance);
    _uint32 op_$minus_minus2(object $instance, var& num);
    _uint32 op_$plus_equal(object $instance, object num);
    _uint32 op_$plus_equal2(object $instance, object num);
    _uint32 op_$plus_equal3(object $instance, object num);
    _uint32 op_$plus_equal4(object $instance, object num);
    _uint32 op_$plus_equal5(object $instance, object num);
    _uint32 op_$plus_equal6(object $instance, object num);
    _uint32 op_$plus_equal7(object $instance, object num);
    _uint32 op_$plus_equal8(object $instance, object num);
    _uint32 op_$plus_equal9(object $instance, object num);
    _uint32 op_$plus_equal10(object $instance, object num);
    _uint32 op_$plus_equal11(object $instance, var& num);
    _uint32 op_$minus_equal(object $instance, object num);
    _uint32 op_$minus_equal2(object $instance, object num);
    _uint32 op_$minus_equal3(object $instance, object num);
    _uint32 op_$minus_equal4(object $instance, object num);
    _uint32 op_$minus_equal5(object $instance, object num);
    _uint32 op_$minus_equal6(object $instance, object num);
    _uint32 op_$minus_equal7(object $instance, object num);
    _uint32 op_$minus_equal8(object $instance, object num);
    _uint32 op_$minus_equal9(object $instance, object num);
    _uint32 op_$minus_equal10(object $instance, object num);
    _uint32 op_$minus_equal11(object $instance, var& num);
    _uint32 op_$mult_equal(object $instance, object num);
    _uint32 op_$mult_equal2(object $instance, object num);
    _uint32 op_$mult_equal3(object $instance, object num);
    _uint32 op_$mult_equal4(object $instance, object num);
    _uint32 op_$mult_equal5(object $instance, object num);
    _uint32 op_$mult_equal6(object $instance, object num);
    _uint32 op_$mult_equal7(object $instance, object num);
    _uint32 op_$mult_equal8(object $instance, object num);
    _uint32 op_$mult_equal9(object $instance, object num);
    _uint32 op_$mult_equal10(object $instance, object num);
    _uint32 op_$mult_equal11(object $instance, var& num);
    _uint32 op_$div_equal(object $instance, object num);
    _uint32 op_$div_equal2(object $instance, object num);
    _uint32 op_$div_equal3(object $instance, object num);
    _uint32 op_$div_equal4(object $instance, object num);
    _uint32 op_$div_equal5(object $instance, object num);
    _uint32 op_$div_equal6(object $instance, object num);
    _uint32 op_$div_equal7(object $instance, object num);
    _uint32 op_$div_equal8(object $instance, object num);
    _uint32 op_$div_equal9(object $instance, object num);
    _uint32 op_$div_equal10(object $instance, object num);
    _uint32 op_$div_equal11(object $instance, var& num);
    _uint32 op_$mod_equal(object $instance, object num);
    _uint32 op_$mod_equal2(object $instance, object num);
    _uint32 op_$mod_equal3(object $instance, object num);
    _uint32 op_$mod_equal4(object $instance, object num);
    _uint32 op_$mod_equal5(object $instance, object num);
    _uint32 op_$mod_equal6(object $instance, object num);
    _uint32 op_$mod_equal7(object $instance, object num);
    _uint32 op_$mod_equal8(object $instance, object num);
    _uint32 op_$mod_equal9(object $instance, object num);
    _uint32 op_$mod_equal10(object $instance, object num);
    _uint32 op_$mod_equal11(object $instance, var& num);
    _uint32 op_$and_equal(object $instance, object num);
    _uint32 op_$and_equal2(object $instance, object num);
    _uint32 op_$and_equal3(object $instance, object num);
    _uint32 op_$and_equal4(object $instance, object num);
    _uint32 op_$and_equal5(object $instance, object num);
    _uint32 op_$and_equal6(object $instance, object num);
    _uint32 op_$and_equal7(object $instance, object num);
    _uint32 op_$and_equal8(object $instance, object num);
    _uint32 op_$and_equal9(object $instance, object num);
    _uint32 op_$and_equal10(object $instance, object num);
    _uint32 op_$and_equal11(object $instance, var& num);
    _uint32 op_$or_equal(object $instance, object num);
    _uint32 op_$or_equal2(object $instance, object num);
    _uint32 op_$or_equal3(object $instance, object num);
    _uint32 op_$or_equal4(object $instance, object num);
    _uint32 op_$or_equal5(object $instance, object num);
    _uint32 op_$or_equal6(object $instance, object num);
    _uint32 op_$or_equal7(object $instance, object num);
    _uint32 op_$or_equal8(object $instance, object num);
    _uint32 op_$or_equal9(object $instance, object num);
    _uint32 op_$or_equal10(object $instance, object num);
    _uint32 op_$or_equal11(object $instance, var& num);
    void uint12(object $instance);
scope_end()

scope_begin(std, _int)

    void _int(object $instance, object i);
    void _int2(object $instance, object i);
    void _int3(object $instance, object i);
    void _int4(object $instance, object i);
    void _int5(object $instance, object i);
    void _int6(object $instance, object i);
    void _int7(object $instance, object i);
    void _int8(object $instance, object i);
    void _int9(object $instance, object i);
    void _int10(object $instance, object i);
    void _int11(object $instance, var& i);
    _int32 op_$not(object $instance);
    _int32 op_$plus_plus(object $instance);
    _int32 op_$plus_plus2(object $instance, var& num);
    _int32 op_$minus_minus(object $instance);
    _int32 op_$minus_minus2(object $instance, var& num);
    _int32 op_$plus_equal(object $instance, object num);
    _int32 op_$plus_equal2(object $instance, object num);
    _int32 op_$plus_equal3(object $instance, object num);
    _int32 op_$plus_equal4(object $instance, object num);
    _int32 op_$plus_equal5(object $instance, object num);
    _int32 op_$plus_equal6(object $instance, object num);
    _int32 op_$plus_equal7(object $instance, object num);
    _int32 op_$plus_equal8(object $instance, object num);
    _int32 op_$plus_equal9(object $instance, object num);
    _int32 op_$plus_equal10(object $instance, object num);
    _int32 op_$plus_equal11(object $instance, var& num);
    _int32 op_$minus_equal(object $instance, object num);
    _int32 op_$minus_equal2(object $instance, object num);
    _int32 op_$minus_equal3(object $instance, object num);
    _int32 op_$minus_equal4(object $instance, object num);
    _int32 op_$minus_equal5(object $instance, object num);
    _int32 op_$minus_equal6(object $instance, object num);
    _int32 op_$minus_equal7(object $instance, object num);
    _int32 op_$minus_equal8(object $instance, object num);
    _int32 op_$minus_equal9(object $instance, object num);
    _int32 op_$minus_equal10(object $instance, object num);
    _int32 op_$minus_equal11(object $instance, var& num);
    _int32 op_$mult_equal(object $instance, object num);
    _int32 op_$mult_equal2(object $instance, object num);
    _int32 op_$mult_equal3(object $instance, object num);
    _int32 op_$mult_equal4(object $instance, object num);
    _int32 op_$mult_equal5(object $instance, object num);
    _int32 op_$mult_equal6(object $instance, object num);
    _int32 op_$mult_equal7(object $instance, object num);
    _int32 op_$mult_equal8(object $instance, object num);
    _int32 op_$mult_equal9(object $instance, object num);
    _int32 op_$mult_equal10(object $instance, object num);
    _int32 op_$mult_equal11(object $instance, var& num);
    _int32 op_$div_equal(object $instance, object num);
    _int32 op_$div_equal2(object $instance, object num);
    _int32 op_$div_equal3(object $instance, object num);
    _int32 op_$div_equal4(object $instance, object num);
    _int32 op_$div_equal5(object $instance, object num);
    _int32 op_$div_equal6(object $instance, object num);
    _int32 op_$div_equal7(object $instance, object num);
    _int32 op_$div_equal8(object $instance, object num);
    _int32 op_$div_equal9(object $instance, object num);
    _int32 op_$div_equal10(object $instance, object num);
    _int32 op_$div_equal11(object $instance, var& num);
    _int32 op_$mod_equal(object $instance, object num);
    _int32 op_$mod_equal2(object $instance, object num);
    _int32 op_$mod_equal3(object $instance, object num);
    _int32 op_$mod_equal4(object $instance, object num);
    _int32 op_$mod_equal5(object $instance, object num);
    _int32 op_$mod_equal6(object $instance, object num);
    _int32 op_$mod_equal7(object $instance, object num);
    _int32 op_$mod_equal8(object $instance, object num);
    _int32 op_$mod_equal9(object $instance, object num);
    _int32 op_$mod_equal10(object $instance, object num);
    _int32 op_$mod_equal11(object $instance, var& num);
    _int32 op_$and_equal(object $instance, object num);
    _int32 op_$and_equal2(object $instance, object num);
    _int32 op_$and_equal3(object $instance, object num);
    _int32 op_$and_equal4(object $instance, object num);
    _int32 op_$and_equal5(object $instance, object num);
    _int32 op_$and_equal6(object $instance, object num);
    _int32 op_$and_equal7(object $instance, object num);
    _int32 op_$and_equal8(object $instance, object num);
    _int32 op_$and_equal9(object $instance, object num);
    _int32 op_$and_equal10(object $instance, object num);
    _int32 op_$and_equal11(object $instance, var& num);
    _int32 op_$or_equal(object $instance, object num);
    _int32 op_$or_equal2(object $instance, object num);
    _int32 op_$or_equal3(object $instance, object num);
    _int32 op_$or_equal4(object $instance, object num);
    _int32 op_$or_equal5(object $instance, object num);
    _int32 op_$or_equal6(object $instance, object num);
    _int32 op_$or_equal7(object $instance, object num);
    _int32 op_$or_equal8(object $instance, object num);
    _int32 op_$or_equal9(object $instance, object num);
    _int32 op_$or_equal10(object $instance, object num);
    _int32 op_$or_equal11(object $instance, var& num);
    void _int12(object $instance);
scope_end()

scope_begin(std, ushort)

    void ushort(object $instance, object i);
    void ushort2(object $instance, object i);
    void ushort3(object $instance, object i);
    void ushort4(object $instance, object i);
    void ushort5(object $instance, object i);
    void ushort6(object $instance, object i);
    void ushort7(object $instance, object i);
    void ushort8(object $instance, object i);
    void ushort9(object $instance, object i);
    void ushort10(object $instance, object i);
    void ushort11(object $instance, var& i);
    _uint16 op_$not(object $instance);
    _uint16 op_$plus_plus(object $instance);
    _uint16 op_$plus_plus2(object $instance, var& num);
    _uint16 op_$minus_minus(object $instance);
    _uint16 op_$minus_minus2(object $instance, var& num);
    _uint16 op_$plus_equal(object $instance, object num);
    _uint16 op_$plus_equal2(object $instance, object num);
    _uint16 op_$plus_equal3(object $instance, object num);
    _uint16 op_$plus_equal4(object $instance, object num);
    _uint16 op_$plus_equal5(object $instance, object num);
    _uint16 op_$plus_equal6(object $instance, object num);
    _uint16 op_$plus_equal7(object $instance, object num);
    _uint16 op_$plus_equal8(object $instance, object num);
    _uint16 op_$plus_equal9(object $instance, object num);
    _uint16 op_$plus_equal10(object $instance, object num);
    _uint16 op_$plus_equal11(object $instance, var& num);
    _uint16 op_$minus_equal(object $instance, object num);
    _uint16 op_$minus_equal2(object $instance, object num);
    _uint16 op_$minus_equal3(object $instance, object num);
    _uint16 op_$minus_equal4(object $instance, object num);
    _uint16 op_$minus_equal5(object $instance, object num);
    _uint16 op_$minus_equal6(object $instance, object num);
    _uint16 op_$minus_equal7(object $instance, object num);
    _uint16 op_$minus_equal8(object $instance, object num);
    _uint16 op_$minus_equal9(object $instance, object num);
    _uint16 op_$minus_equal10(object $instance, object num);
    _uint16 op_$minus_equal11(object $instance, var& num);
    _uint16 op_$mult_equal(object $instance, object num);
    _uint16 op_$mult_equal2(object $instance, object num);
    _uint16 op_$mult_equal3(object $instance, object num);
    _uint16 op_$mult_equal4(object $instance, object num);
    _uint16 op_$mult_equal5(object $instance, object num);
    _uint16 op_$mult_equal6(object $instance, object num);
    _uint16 op_$mult_equal7(object $instance, object num);
    _uint16 op_$mult_equal8(object $instance, object num);
    _uint16 op_$mult_equal9(object $instance, object num);
    _uint16 op_$mult_equal10(object $instance, object num);
    _uint16 op_$mult_equal11(object $instance, var& num);
    _uint16 op_$div_equal(object $instance, object num);
    _uint16 op_$div_equal2(object $instance, object num);
    _uint16 op_$div_equal3(object $instance, object num);
    _uint16 op_$div_equal4(object $instance, object num);
    _uint16 op_$div_equal5(object $instance, object num);
    _uint16 op_$div_equal6(object $instance, object num);
    _uint16 op_$div_equal7(object $instance, object num);
    _uint16 op_$div_equal8(object $instance, object num);
    _uint16 op_$div_equal9(object $instance, object num);
    _uint16 op_$div_equal10(object $instance, object num);
    _uint16 op_$div_equal11(object $instance, var& num);
    _uint16 op_$mod_equal(object $instance, object num);
    _uint16 op_$mod_equal2(object $instance, object num);
    _uint16 op_$mod_equal3(object $instance, object num);
    _uint16 op_$mod_equal4(object $instance, object num);
    _uint16 op_$mod_equal5(object $instance, object num);
    _uint16 op_$mod_equal6(object $instance, object num);
    _uint16 op_$mod_equal7(object $instance, object num);
    _uint16 op_$mod_equal8(object $instance, object num);
    _uint16 op_$mod_equal9(object $instance, object num);
    _uint16 op_$mod_equal10(object $instance, object num);
    _uint16 op_$mod_equal11(object $instance, var& num);
    _uint16 op_$and_equal(object $instance, object num);
    _uint16 op_$and_equal2(object $instance, object num);
    _uint16 op_$and_equal3(object $instance, object num);
    _uint16 op_$and_equal4(object $instance, object num);
    _uint16 op_$and_equal5(object $instance, object num);
    _uint16 op_$and_equal6(object $instance, object num);
    _uint16 op_$and_equal7(object $instance, object num);
    _uint16 op_$and_equal8(object $instance, object num);
    _uint16 op_$and_equal9(object $instance, object num);
    _uint16 op_$and_equal10(object $instance, object num);
    _uint16 op_$and_equal11(object $instance, var& num);
    _uint16 op_$or_equal(object $instance, object num);
    _uint16 op_$or_equal2(object $instance, object num);
    _uint16 op_$or_equal3(object $instance, object num);
    _uint16 op_$or_equal4(object $instance, object num);
    _uint16 op_$or_equal5(object $instance, object num);
    _uint16 op_$or_equal6(object $instance, object num);
    _uint16 op_$or_equal7(object $instance, object num);
    _uint16 op_$or_equal8(object $instance, object num);
    _uint16 op_$or_equal9(object $instance, object num);
    _uint16 op_$or_equal10(object $instance, object num);
    _uint16 op_$or_equal11(object $instance, var& num);
    void ushort12(object $instance);
scope_end()

scope_begin(std, _short)

    void _short(object $instance, object i);
    void _short2(object $instance, object i);
    void _short3(object $instance, object i);
    void _short4(object $instance, object i);
    void _short5(object $instance, object i);
    void _short6(object $instance, object i);
    void _short7(object $instance, object i);
    void _short8(object $instance, object i);
    void _short9(object $instance, object i);
    void _short10(object $instance, object i);
    void _short11(object $instance, var& i);
    _int16 op_$not(object $instance);
    _int16 op_$plus_plus(object $instance);
    _int16 op_$plus_plus2(object $instance, var& num);
    _int16 op_$minus_minus(object $instance);
    _int16 op_$minus_minus2(object $instance, var& num);
    _int16 op_$plus_equal(object $instance, object num);
    _int16 op_$plus_equal2(object $instance, object num);
    _int16 op_$plus_equal3(object $instance, object num);
    _int16 op_$plus_equal4(object $instance, object num);
    _int16 op_$plus_equal5(object $instance, object num);
    _int16 op_$plus_equal6(object $instance, object num);
    _int16 op_$plus_equal7(object $instance, object num);
    _int16 op_$plus_equal8(object $instance, object num);
    _int16 op_$plus_equal9(object $instance, object num);
    _int16 op_$plus_equal10(object $instance, object num);
    _int16 op_$plus_equal11(object $instance, var& num);
    _int16 op_$minus_equal(object $instance, object num);
    _int16 op_$minus_equal2(object $instance, object num);
    _int16 op_$minus_equal3(object $instance, object num);
    _int16 op_$minus_equal4(object $instance, object num);
    _int16 op_$minus_equal5(object $instance, object num);
    _int16 op_$minus_equal6(object $instance, object num);
    _int16 op_$minus_equal7(object $instance, object num);
    _int16 op_$minus_equal8(object $instance, object num);
    _int16 op_$minus_equal9(object $instance, object num);
    _int16 op_$minus_equal10(object $instance, object num);
    _int16 op_$minus_equal11(object $instance, var& num);
    _int16 op_$mult_equal(object $instance, object num);
    _int16 op_$mult_equal2(object $instance, object num);
    _int16 op_$mult_equal3(object $instance, object num);
    _int16 op_$mult_equal4(object $instance, object num);
    _int16 op_$mult_equal5(object $instance, object num);
    _int16 op_$mult_equal6(object $instance, object num);
    _int16 op_$mult_equal7(object $instance, object num);
    _int16 op_$mult_equal8(object $instance, object num);
    _int16 op_$mult_equal9(object $instance, object num);
    _int16 op_$mult_equal10(object $instance, object num);
    _int16 op_$mult_equal11(object $instance, var& num);
    _int16 op_$div_equal(object $instance, object num);
    _int16 op_$div_equal2(object $instance, object num);
    _int16 op_$div_equal3(object $instance, object num);
    _int16 op_$div_equal4(object $instance, object num);
    _int16 op_$div_equal5(object $instance, object num);
    _int16 op_$div_equal6(object $instance, object num);
    _int16 op_$div_equal7(object $instance, object num);
    _int16 op_$div_equal8(object $instance, object num);
    _int16 op_$div_equal9(object $instance, object num);
    _int16 op_$div_equal10(object $instance, object num);
    _int16 op_$div_equal11(object $instance, var& num);
    _int16 op_$mod_equal(object $instance, object num);
    _int16 op_$mod_equal2(object $instance, object num);
    _int16 op_$mod_equal3(object $instance, object num);
    _int16 op_$mod_equal4(object $instance, object num);
    _int16 op_$mod_equal5(object $instance, object num);
    _int16 op_$mod_equal6(object $instance, object num);
    _int16 op_$mod_equal7(object $instance, object num);
    _int16 op_$mod_equal8(object $instance, object num);
    _int16 op_$mod_equal9(object $instance, object num);
    _int16 op_$mod_equal10(object $instance, object num);
    _int16 op_$mod_equal11(object $instance, var& num);
    _int16 op_$and_equal(object $instance, object num);
    _int16 op_$and_equal2(object $instance, object num);
    _int16 op_$and_equal3(object $instance, object num);
    _int16 op_$and_equal4(object $instance, object num);
    _int16 op_$and_equal5(object $instance, object num);
    _int16 op_$and_equal6(object $instance, object num);
    _int16 op_$and_equal7(object $instance, object num);
    _int16 op_$and_equal8(object $instance, object num);
    _int16 op_$and_equal9(object $instance, object num);
    _int16 op_$and_equal10(object $instance, object num);
    _int16 op_$and_equal11(object $instance, var& num);
    _int16 op_$or_equal(object $instance, object num);
    _int16 op_$or_equal2(object $instance, object num);
    _int16 op_$or_equal3(object $instance, object num);
    _int16 op_$or_equal4(object $instance, object num);
    _int16 op_$or_equal5(object $instance, object num);
    _int16 op_$or_equal6(object $instance, object num);
    _int16 op_$or_equal7(object $instance, object num);
    _int16 op_$or_equal8(object $instance, object num);
    _int16 op_$or_equal9(object $instance, object num);
    _int16 op_$or_equal10(object $instance, object num);
    _int16 op_$or_equal11(object $instance, var& num);
    void _short12(object $instance);
scope_end()

scope_begin(std, uchar)

    void uchar(object $instance, object i);
    void uchar2(object $instance, object i);
    void uchar3(object $instance, object i);
    void uchar4(object $instance, object i);
    void uchar5(object $instance, object i);
    void uchar6(object $instance, object i);
    void uchar7(object $instance, object i);
    void uchar8(object $instance, object i);
    void uchar9(object $instance, object i);
    void uchar10(object $instance, object i);
    void uchar11(object $instance, var& i);
    _uint8 op_$not(object $instance);
    _uint8 op_$plus_plus(object $instance);
    _uint8 op_$plus_plus2(object $instance, var& num);
    _uint8 op_$minus_minus(object $instance);
    _uint8 op_$minus_minus2(object $instance, var& num);
    _uint8 op_$plus_equal(object $instance, object num);
    _uint8 op_$plus_equal2(object $instance, object num);
    _uint8 op_$plus_equal3(object $instance, object num);
    _uint8 op_$plus_equal4(object $instance, object num);
    _uint8 op_$plus_equal5(object $instance, object num);
    _uint8 op_$plus_equal6(object $instance, object num);
    _uint8 op_$plus_equal7(object $instance, object num);
    _uint8 op_$plus_equal8(object $instance, object num);
    _uint8 op_$plus_equal9(object $instance, object num);
    _uint8 op_$plus_equal10(object $instance, object num);
    _uint8 op_$plus_equal11(object $instance, var& num);
    _uint8 op_$minus_equal(object $instance, object num);
    _uint8 op_$minus_equal2(object $instance, object num);
    _uint8 op_$minus_equal3(object $instance, object num);
    _uint8 op_$minus_equal4(object $instance, object num);
    _uint8 op_$minus_equal5(object $instance, object num);
    _uint8 op_$minus_equal6(object $instance, object num);
    _uint8 op_$minus_equal7(object $instance, object num);
    _uint8 op_$minus_equal8(object $instance, object num);
    _uint8 op_$minus_equal9(object $instance, object num);
    _uint8 op_$minus_equal10(object $instance, object num);
    _uint8 op_$minus_equal11(object $instance, var& num);
    _uint8 op_$mult_equal(object $instance, object num);
    _uint8 op_$mult_equal2(object $instance, object num);
    _uint8 op_$mult_equal3(object $instance, object num);
    _uint8 op_$mult_equal4(object $instance, object num);
    _uint8 op_$mult_equal5(object $instance, object num);
    _uint8 op_$mult_equal6(object $instance, object num);
    _uint8 op_$mult_equal7(object $instance, object num);
    _uint8 op_$mult_equal8(object $instance, object num);
    _uint8 op_$mult_equal9(object $instance, object num);
    _uint8 op_$mult_equal10(object $instance, object num);
    _uint8 op_$mult_equal11(object $instance, var& num);
    _uint8 op_$div_equal(object $instance, object num);
    _uint8 op_$div_equal2(object $instance, object num);
    _uint8 op_$div_equal3(object $instance, object num);
    _uint8 op_$div_equal4(object $instance, object num);
    _uint8 op_$div_equal5(object $instance, object num);
    _uint8 op_$div_equal6(object $instance, object num);
    _uint8 op_$div_equal7(object $instance, object num);
    _uint8 op_$div_equal8(object $instance, object num);
    _uint8 op_$div_equal9(object $instance, object num);
    _uint8 op_$div_equal10(object $instance, object num);
    _uint8 op_$div_equal11(object $instance, var& num);
    _uint8 op_$mod_equal(object $instance, object num);
    _uint8 op_$mod_equal2(object $instance, object num);
    _uint8 op_$mod_equal3(object $instance, object num);
    _uint8 op_$mod_equal4(object $instance, object num);
    _uint8 op_$mod_equal5(object $instance, object num);
    _uint8 op_$mod_equal6(object $instance, object num);
    _uint8 op_$mod_equal7(object $instance, object num);
    _uint8 op_$mod_equal8(object $instance, object num);
    _uint8 op_$mod_equal9(object $instance, object num);
    _uint8 op_$mod_equal10(object $instance, object num);
    _uint8 op_$mod_equal11(object $instance, var& num);
    _uint8 op_$and_equal(object $instance, object num);
    _uint8 op_$and_equal2(object $instance, object num);
    _uint8 op_$and_equal3(object $instance, object num);
    _uint8 op_$and_equal4(object $instance, object num);
    _uint8 op_$and_equal5(object $instance, object num);
    _uint8 op_$and_equal6(object $instance, object num);
    _uint8 op_$and_equal7(object $instance, object num);
    _uint8 op_$and_equal8(object $instance, object num);
    _uint8 op_$and_equal9(object $instance, object num);
    _uint8 op_$and_equal10(object $instance, object num);
    _uint8 op_$and_equal11(object $instance, var& num);
    _uint8 op_$or_equal(object $instance, object num);
    _uint8 op_$or_equal2(object $instance, object num);
    _uint8 op_$or_equal3(object $instance, object num);
    _uint8 op_$or_equal4(object $instance, object num);
    _uint8 op_$or_equal5(object $instance, object num);
    _uint8 op_$or_equal6(object $instance, object num);
    _uint8 op_$or_equal7(object $instance, object num);
    _uint8 op_$or_equal8(object $instance, object num);
    _uint8 op_$or_equal9(object $instance, object num);
    _uint8 op_$or_equal10(object $instance, object num);
    _uint8 op_$or_equal11(object $instance, var& num);
    void uchar12(object $instance);
scope_end()

scope_begin(std, _char)

    void _char(object $instance, object i);
    void _char2(object $instance, object i);
    void _char3(object $instance, object i);
    void _char4(object $instance, object i);
    void _char5(object $instance, object i);
    void _char6(object $instance, object i);
    void _char7(object $instance, object i);
    void _char8(object $instance, object i);
    void _char9(object $instance, object i);
    void _char10(object $instance, object i);
    void _char11(object $instance, var& i);
    _int8 op_$not(object $instance);
    _int8 op_$plus_plus(object $instance);
    _int8 op_$plus_plus2(object $instance, var& num);
    _int8 op_$minus_minus(object $instance);
    _int8 op_$minus_minus2(object $instance, var& num);
    _int8 op_$plus_equal(object $instance, object num);
    _int8 op_$plus_equal2(object $instance, object num);
    _int8 op_$plus_equal3(object $instance, object num);
    _int8 op_$plus_equal4(object $instance, object num);
    _int8 op_$plus_equal5(object $instance, object num);
    _int8 op_$plus_equal6(object $instance, object num);
    _int8 op_$plus_equal7(object $instance, object num);
    _int8 op_$plus_equal8(object $instance, object num);
    _int8 op_$plus_equal9(object $instance, object num);
    _int8 op_$plus_equal10(object $instance, object num);
    _int8 op_$plus_equal11(object $instance, var& num);
    _int8 op_$minus_equal(object $instance, object num);
    _int8 op_$minus_equal2(object $instance, object num);
    _int8 op_$minus_equal3(object $instance, object num);
    _int8 op_$minus_equal4(object $instance, object num);
    _int8 op_$minus_equal5(object $instance, object num);
    _int8 op_$minus_equal6(object $instance, object num);
    _int8 op_$minus_equal7(object $instance, object num);
    _int8 op_$minus_equal8(object $instance, object num);
    _int8 op_$minus_equal9(object $instance, object num);
    _int8 op_$minus_equal10(object $instance, object num);
    _int8 op_$minus_equal11(object $instance, var& num);
    _int8 op_$mult_equal(object $instance, object num);
    _int8 op_$mult_equal2(object $instance, object num);
    _int8 op_$mult_equal3(object $instance, object num);
    _int8 op_$mult_equal4(object $instance, object num);
    _int8 op_$mult_equal5(object $instance, object num);
    _int8 op_$mult_equal6(object $instance, object num);
    _int8 op_$mult_equal7(object $instance, object num);
    _int8 op_$mult_equal8(object $instance, object num);
    _int8 op_$mult_equal9(object $instance, object num);
    _int8 op_$mult_equal10(object $instance, object num);
    _int8 op_$mult_equal11(object $instance, var& num);
    _int8 op_$div_equal(object $instance, object num);
    _int8 op_$div_equal2(object $instance, object num);
    _int8 op_$div_equal3(object $instance, object num);
    _int8 op_$div_equal4(object $instance, object num);
    _int8 op_$div_equal5(object $instance, object num);
    _int8 op_$div_equal6(object $instance, object num);
    _int8 op_$div_equal7(object $instance, object num);
    _int8 op_$div_equal8(object $instance, object num);
    _int8 op_$div_equal9(object $instance, object num);
    _int8 op_$div_equal10(object $instance, object num);
    _int8 op_$div_equal11(object $instance, var& num);
    _int8 op_$mod_equal(object $instance, object num);
    _int8 op_$mod_equal2(object $instance, object num);
    _int8 op_$mod_equal3(object $instance, object num);
    _int8 op_$mod_equal4(object $instance, object num);
    _int8 op_$mod_equal5(object $instance, object num);
    _int8 op_$mod_equal6(object $instance, object num);
    _int8 op_$mod_equal7(object $instance, object num);
    _int8 op_$mod_equal8(object $instance, object num);
    _int8 op_$mod_equal9(object $instance, object num);
    _int8 op_$mod_equal10(object $instance, object num);
    _int8 op_$mod_equal11(object $instance, var& num);
    _int8 op_$and_equal(object $instance, object num);
    _int8 op_$and_equal2(object $instance, object num);
    _int8 op_$and_equal3(object $instance, object num);
    _int8 op_$and_equal4(object $instance, object num);
    _int8 op_$and_equal5(object $instance, object num);
    _int8 op_$and_equal6(object $instance, object num);
    _int8 op_$and_equal7(object $instance, object num);
    _int8 op_$and_equal8(object $instance, object num);
    _int8 op_$and_equal9(object $instance, object num);
    _int8 op_$and_equal10(object $instance, object num);
    _int8 op_$and_equal11(object $instance, var& num);
    _int8 op_$or_equal(object $instance, object num);
    _int8 op_$or_equal2(object $instance, object num);
    _int8 op_$or_equal3(object $instance, object num);
    _int8 op_$or_equal4(object $instance, object num);
    _int8 op_$or_equal5(object $instance, object num);
    _int8 op_$or_equal6(object $instance, object num);
    _int8 op_$or_equal7(object $instance, object num);
    _int8 op_$or_equal8(object $instance, object num);
    _int8 op_$or_equal9(object $instance, object num);
    _int8 op_$or_equal10(object $instance, object num);
    _int8 op_$or_equal11(object $instance, var& num);
    var is_digit(var& ch);
    var is_newline(var& ch);
    var is_alpha(var& ch);
    var is_alpha_num(var& ch);
    var to_lower(var& ch);
    var to_upper(var& ch);
    void _char12(object $instance);
scope_end()

scope_begin(std, byte)

    void byte(object $instance, object i);
    void byte2(object $instance, object i);
    void byte3(object $instance, object i);
    void byte4(object $instance, object i);
    void byte5(object $instance, object i);
    void byte6(object $instance, object i);
    void byte7(object $instance, object i);
    void byte8(object $instance, object i);
    void byte9(object $instance, object i);
    void byte10(object $instance, object i);
    void byte11(object $instance, var& i);
    _uint8 op_$not(object $instance);
    _uint8 op_$plus_plus(object $instance);
    _uint8 op_$plus_plus2(object $instance, var& num);
    _uint8 op_$minus_minus(object $instance);
    _uint8 op_$minus_minus2(object $instance, var& num);
    _uint8 op_$plus_equal(object $instance, object num);
    _uint8 op_$plus_equal2(object $instance, object num);
    _uint8 op_$plus_equal3(object $instance, object num);
    _uint8 op_$plus_equal4(object $instance, object num);
    _uint8 op_$plus_equal5(object $instance, object num);
    _uint8 op_$plus_equal6(object $instance, object num);
    _uint8 op_$plus_equal7(object $instance, object num);
    _uint8 op_$plus_equal8(object $instance, object num);
    _uint8 op_$plus_equal9(object $instance, object num);
    _uint8 op_$plus_equal10(object $instance, object num);
    _uint8 op_$plus_equal11(object $instance, var& num);
    _uint8 op_$minus_equal(object $instance, object num);
    _uint8 op_$minus_equal2(object $instance, object num);
    _uint8 op_$minus_equal3(object $instance, object num);
    _uint8 op_$minus_equal4(object $instance, object num);
    _uint8 op_$minus_equal5(object $instance, object num);
    _uint8 op_$minus_equal6(object $instance, object num);
    _uint8 op_$minus_equal7(object $instance, object num);
    _uint8 op_$minus_equal8(object $instance, object num);
    _uint8 op_$minus_equal9(object $instance, object num);
    _uint8 op_$minus_equal10(object $instance, object num);
    _uint8 op_$minus_equal11(object $instance, var& num);
    _uint8 op_$mult_equal(object $instance, object num);
    _uint8 op_$mult_equal2(object $instance, object num);
    _uint8 op_$mult_equal3(object $instance, object num);
    _uint8 op_$mult_equal4(object $instance, object num);
    _uint8 op_$mult_equal5(object $instance, object num);
    _uint8 op_$mult_equal6(object $instance, object num);
    _uint8 op_$mult_equal7(object $instance, object num);
    _uint8 op_$mult_equal8(object $instance, object num);
    _uint8 op_$mult_equal9(object $instance, object num);
    _uint8 op_$mult_equal10(object $instance, object num);
    _uint8 op_$mult_equal11(object $instance, var& num);
    _uint8 op_$div_equal(object $instance, object num);
    _uint8 op_$div_equal2(object $instance, object num);
    _uint8 op_$div_equal3(object $instance, object num);
    _uint8 op_$div_equal4(object $instance, object num);
    _uint8 op_$div_equal5(object $instance, object num);
    _uint8 op_$div_equal6(object $instance, object num);
    _uint8 op_$div_equal7(object $instance, object num);
    _uint8 op_$div_equal8(object $instance, object num);
    _uint8 op_$div_equal9(object $instance, object num);
    _uint8 op_$div_equal10(object $instance, object num);
    _uint8 op_$div_equal11(object $instance, var& num);
    _uint8 op_$mod_equal(object $instance, object num);
    _uint8 op_$mod_equal2(object $instance, object num);
    _uint8 op_$mod_equal3(object $instance, object num);
    _uint8 op_$mod_equal4(object $instance, object num);
    _uint8 op_$mod_equal5(object $instance, object num);
    _uint8 op_$mod_equal6(object $instance, object num);
    _uint8 op_$mod_equal7(object $instance, object num);
    _uint8 op_$mod_equal8(object $instance, object num);
    _uint8 op_$mod_equal9(object $instance, object num);
    _uint8 op_$mod_equal10(object $instance, object num);
    _uint8 op_$mod_equal11(object $instance, var& num);
    _uint8 op_$and_equal(object $instance, object num);
    _uint8 op_$and_equal2(object $instance, object num);
    _uint8 op_$and_equal3(object $instance, object num);
    _uint8 op_$and_equal4(object $instance, object num);
    _uint8 op_$and_equal5(object $instance, object num);
    _uint8 op_$and_equal6(object $instance, object num);
    _uint8 op_$and_equal7(object $instance, object num);
    _uint8 op_$and_equal8(object $instance, object num);
    _uint8 op_$and_equal9(object $instance, object num);
    _uint8 op_$and_equal10(object $instance, object num);
    _uint8 op_$and_equal11(object $instance, var& num);
    _uint8 op_$or_equal(object $instance, object num);
    _uint8 op_$or_equal2(object $instance, object num);
    _uint8 op_$or_equal3(object $instance, object num);
    _uint8 op_$or_equal4(object $instance, object num);
    _uint8 op_$or_equal5(object $instance, object num);
    _uint8 op_$or_equal6(object $instance, object num);
    _uint8 op_$or_equal7(object $instance, object num);
    _uint8 op_$or_equal8(object $instance, object num);
    _uint8 op_$or_equal9(object $instance, object num);
    _uint8 op_$or_equal10(object $instance, object num);
    _uint8 op_$or_equal11(object $instance, var& num);
    var is_digit(var& ch);
    var is_newline(var& ch);
    var is_alpha(var& ch);
    var is_alpha_num(var& ch);
    var to_lower(var& ch);
    var to_upper(var& ch);
    void byte12(object $instance);
scope_end()

scope_begin(std, _bool)

    void _bool(object $instance, object i);
    void _bool2(object $instance, object i);
    void _bool3(object $instance, object i);
    void _bool4(object $instance, object i);
    void _bool5(object $instance, object i);
    void _bool6(object $instance, object i);
    void _bool7(object $instance, object i);
    void _bool8(object $instance, object i);
    void _bool9(object $instance, object i);
    void _bool10(object $instance, object i);
    void _bool11(object $instance, var& i);
    _int8 op_$not(object $instance);
    _int8 op_$plus_plus(object $instance);
    _int8 op_$plus_plus2(object $instance, var& num);
    _int8 op_$minus_minus(object $instance);
    _int8 op_$minus_minus2(object $instance, var& num);
    var op_$plus_equal(object $instance, object num);
    var op_$plus_equal2(object $instance, object num);
    var op_$plus_equal3(object $instance, object num);
    var op_$plus_equal4(object $instance, object num);
    var op_$plus_equal5(object $instance, object num);
    var op_$plus_equal6(object $instance, object num);
    var op_$plus_equal7(object $instance, object num);
    var op_$plus_equal8(object $instance, object num);
    var op_$plus_equal9(object $instance, object num);
    var op_$plus_equal10(object $instance, object num);
    var op_$plus_equal11(object $instance, var& num);
    var op_$minus_equal(object $instance, object num);
    var op_$minus_equal2(object $instance, object num);
    var op_$minus_equal3(object $instance, object num);
    var op_$minus_equal4(object $instance, object num);
    var op_$minus_equal5(object $instance, object num);
    var op_$minus_equal6(object $instance, object num);
    var op_$minus_equal7(object $instance, object num);
    var op_$minus_equal8(object $instance, object num);
    var op_$minus_equal9(object $instance, object num);
    var op_$minus_equal10(object $instance, object num);
    var op_$minus_equal11(object $instance, var& num);
    var op_$mult_equal(object $instance, object num);
    var op_$mult_equal2(object $instance, object num);
    var op_$mult_equal3(object $instance, object num);
    var op_$mult_equal4(object $instance, object num);
    var op_$mult_equal5(object $instance, object num);
    var op_$mult_equal6(object $instance, object num);
    var op_$mult_equal7(object $instance, object num);
    var op_$mult_equal8(object $instance, object num);
    var op_$mult_equal9(object $instance, object num);
    var op_$mult_equal10(object $instance, object num);
    var op_$mult_equal11(object $instance, var& num);
    var op_$div_equal(object $instance, object num);
    var op_$div_equal2(object $instance, object num);
    var op_$div_equal3(object $instance, object num);
    var op_$div_equal4(object $instance, object num);
    var op_$div_equal5(object $instance, object num);
    var op_$div_equal6(object $instance, object num);
    var op_$div_equal7(object $instance, object num);
    var op_$div_equal8(object $instance, object num);
    var op_$div_equal9(object $instance, object num);
    var op_$div_equal10(object $instance, object num);
    var op_$div_equal11(object $instance, var& num);
    var op_$mod_equal(object $instance, object num);
    var op_$mod_equal2(object $instance, object num);
    var op_$mod_equal3(object $instance, object num);
    var op_$mod_equal4(object $instance, object num);
    var op_$mod_equal5(object $instance, object num);
    var op_$mod_equal6(object $instance, object num);
    var op_$mod_equal7(object $instance, object num);
    var op_$mod_equal8(object $instance, object num);
    var op_$mod_equal9(object $instance, object num);
    var op_$mod_equal10(object $instance, object num);
    var op_$mod_equal11(object $instance, var& num);
    var op_$and_equal(object $instance, object num);
    var op_$and_equal2(object $instance, object num);
    var op_$and_equal3(object $instance, object num);
    var op_$and_equal4(object $instance, object num);
    var op_$and_equal5(object $instance, object num);
    var op_$and_equal6(object $instance, object num);
    var op_$and_equal7(object $instance, object num);
    var op_$and_equal8(object $instance, object num);
    var op_$and_equal9(object $instance, object num);
    var op_$and_equal10(object $instance, object num);
    var op_$and_equal11(object $instance, var& num);
    var op_$or_equal(object $instance, object num);
    var op_$or_equal2(object $instance, object num);
    var op_$or_equal3(object $instance, object num);
    var op_$or_equal4(object $instance, object num);
    var op_$or_equal5(object $instance, object num);
    var op_$or_equal6(object $instance, object num);
    var op_$or_equal7(object $instance, object num);
    var op_$or_equal8(object $instance, object num);
    var op_$or_equal9(object $instance, object num);
    var op_$or_equal10(object $instance, object num);
    var op_$or_equal11(object $instance, var& num);
    var check_value(object $instance, var& new_val);
    void _bool12(object $instance);
scope_end()

scope_begin(std, printable)

    object to_string(object $instance);
    void printable(object $instance);
scope_end()

scope_begin(std_reflect)

    void __srt_global(object $instance);
scope_end()

scope_begin(std_reflect, _class_)

    void _class_(object $instance);
scope_end()

scope_begin(std_reflect, data_entity)

    void data_entity(object $instance);
scope_end()

scope_begin(std_reflect, data_property)

    void data_property(object $instance);
scope_end()

scope_begin(std_reflect, field)

    void field(object $instance);
scope_end()

scope_begin(std_reflect, function)

    void function(object $instance);
scope_end()

scope_begin(std_reflect, reflect)

    void reflect(object $instance, object ref_obj);
    void reflect2(object $instance);
    void set_reflect_obj(object $instance, object value);
    object get_reflect_obj(object $instance);
scope_end()

scope_begin(std, string)

    void string(object $instance, _int8_array& str);
    void string2(object $instance, var& ch);
    void string3(object $instance, object str);
    void string4(object $instance, object str);
    void string5(object $instance, _int8_array& str, var& offset);
    void string6(object $instance, _int8_array& str, var& start, var& end);
    object replace(object $instance, var& pos, _int8& ch);
    object append(object $instance, var& ch);
    var empty(object $instance);
    object append2(object $instance, object c);
    object append3(object $instance, _int8_array& immstr);
    object append4(object $instance, _int8_array& immstr, var& len);
    object op_$plus(object $instance, _int8_array& immstr);
    object to_lower(object $instance);
    object op_$plus2(object $instance, object obj);
    object op_$plus3(object $instance, object str);
    object to_string(object $instance);
    var op_$equals_equals(object $instance, _int8_array& str);
    var op_$equals_equals2(object $instance, object str);
    var op_$not_equals(object $instance, _int8_array& str);
    var op_$not_equals2(object $instance, object str);
    void op_$equals(object $instance, _int8_array& immstr);
    _int8 at(object $instance, var& pos);
    _int8 op_$array_at(object $instance, var& pos);
    void fmt(var& _fmt);
    void set_precision(var& prec);
    _int8_array get_data(object $instance);
    _int8_array get_elements(object $instance);
    _int8_array to_array(object $instance);
    var starts_with(object $instance, object prefix);
    var find(object $instance, object str);
    var ends_with(object $instance, object postfix);
    object op_$plus4(object $instance, var& num);
    var size(object $instance);
    object parse(var& number);
    object op_$plus_equal(object $instance, var& num);
    object op_$plus_equal2(object $instance, _int8& num);
    object op_$plus_equal3(object $instance, _uint8& num);
    object op_$plus_equal4(object $instance, _int16& num);
    object op_$plus_equal5(object $instance, _uint16& num);
    object op_$plus_equal6(object $instance, _int32& num);
    object op_$plus_equal7(object $instance, _uint32& num);
    object op_$plus_equal8(object $instance, _int64& num);
    object op_$plus_equal9(object $instance, _uint64& num);
    object op_$plus_equal10(object $instance, object obj);
    object op_$plus_equal11(object $instance, object str);
    object op_$plus_equal12(object $instance, _int8_array& str);
    object substring(object $instance, var& start_pos, var& end_pos);
    object bounds_error(object $instance, var& pos);
    object hash(object $instance);
    void string7(object $instance);
    void $03internal_static_init();
scope_end()

scope_begin(std, string_builder)

    void set_resize_capacity(object $instance, var& new_capacity);
    void string_builder(object $instance, _int8_array& str);
    void string_builder2(object $instance, var& ch);
    void string_builder3(object $instance, object str);
    void string_builder4(object $instance, object str);
    void string_builder5(object $instance, object str);
    void string_builder6(object $instance, _int8_array& str, var& offset);
    var space_available(object $instance, var& space_required);
    void expand(object $instance, var& space_required);
    object replace(object $instance, var& pos, _int8& ch);
    object append(object $instance, var& ch);
    _int8_array get_elements(object $instance);
    object append2(object $instance, object c);
    object append3(object $instance, _int8_array& immstr);
    object append4(object $instance, _int8_array& immstr, var& len);
    object op_$plus(object $instance, _int8_array& immstr);
    object op_$plus2(object $instance, object obj);
    object to_string(object $instance);
    var op_$equals_equals(object $instance, _int8_array& str);
    var op_$equals_equals2(object $instance, object str);
    var op_$not_equals(object $instance, _int8_array& str);
    var op_$not_equals2(object $instance, object str);
    void op_$equals(object $instance, _int8_array& immstr);
    _int8 at(object $instance, var& pos);
    var op_$array_at(object $instance, var& pos);
    void fmt(var& _fmt);
    void set_precision(var& prec);
    _int8_array to_array(object $instance);
    var starts_with(object $instance, object prefix);
    var find(object $instance, object str);
    var find2(object $instance, object str);
    var ends_with(object $instance, object postfix);
    object op_$plus3(object $instance, var& num);
    var size(object $instance);
    object parse(object $instance, var& number);
    object op_$plus_equal(object $instance, var& num);
    object op_$plus_equal2(object $instance, object obj);
    object op_$plus_equal3(object $instance, object str);
    _int8_array get_data(object $instance);
    object op_$plus_equal4(object $instance, object str);
    object op_$plus_equal5(object $instance, _int8_array& str);
    object substring(object $instance, var& startPos, var& endPos);
    object bounds_error(object $instance, var& pos);
    object hash(object $instance);
    void string_builder7(object $instance);
    void $03internal_static_init();
scope_end()

scope_begin(std, time)

    var nano_time();
    var nano_to_micro(var& nano);
    var nano_to_mills(var& nano);
    var nano_to_secs(var& nano);
    var mills_time();
    void sleep(var& time);
    void sleep2(object tm_unit, var& time);
    void wait(object tm_unit, var& time);
    var adjust_time(var& usec);
    var measure(object tm_unit, var& block);
    void time(object $instance);
    void $03internal_static_init();
scope_end()

scope_begin(std, unique)

    object hash(object $instance);
    object guid(object $instance);
    void unique(object $instance);
scope_end()

scope_begin(std, loopable$std_io_thread$)

    object get_elements(object $instance);
    void loopable(object $instance);
scope_end()

scope_begin(std, integer$_uint64$)

    void integer(object $instance, var& initial_val);
    object hash(object $instance);
    var op_$not_equals(object $instance, object num);
    var op_$equals_equals(object $instance, object num);
    var op_$not_equals2(object $instance, object num);
    var op_$equals_equals2(object $instance, object num);
    var op_$not_equals3(object $instance, object num);
    var op_$equals_equals3(object $instance, object num);
    var op_$not_equals4(object $instance, object num);
    var op_$equals_equals4(object $instance, object num);
    var op_$not_equals5(object $instance, object num);
    var op_$equals_equals5(object $instance, object num);
    var op_$not_equals6(object $instance, object num);
    var op_$equals_equals6(object $instance, object num);
    var op_$not_equals7(object $instance, object num);
    var op_$equals_equals7(object $instance, object num);
    var op_$not_equals8(object $instance, object num);
    var op_$equals_equals8(object $instance, object num);
    var op_$not_equals9(object $instance, object num);
    var op_$equals_equals9(object $instance, object num);
    var op_$not_equals10(object $instance, object num);
    var op_$equals_equals10(object $instance, object num);
    var op_$plus(object $instance, object num);
    var op_$plus2(object $instance, object num);
    var op_$plus3(object $instance, object num);
    var op_$plus4(object $instance, object num);
    var op_$plus5(object $instance, object num);
    var op_$plus6(object $instance, object num);
    var op_$plus7(object $instance, object num);
    var op_$plus8(object $instance, object num);
    var op_$plus9(object $instance, object num);
    var op_$plus10(object $instance, object num);
    var op_$plus11(object $instance, var& num);
    var op_$minus(object $instance, object num);
    var op_$minus2(object $instance, object num);
    var op_$minus3(object $instance, object num);
    var op_$minus4(object $instance, object num);
    var op_$minus5(object $instance, object num);
    var op_$minus6(object $instance, object num);
    var op_$minus7(object $instance, object num);
    var op_$minus8(object $instance, object num);
    var op_$minus9(object $instance, object num);
    var op_$minus10(object $instance, object num);
    var op_$minus11(object $instance, var& num);
    var op_$mult(object $instance, object num);
    var op_$mult2(object $instance, object num);
    var op_$mult3(object $instance, object num);
    var op_$mult4(object $instance, object num);
    var op_$mult5(object $instance, object num);
    var op_$mult6(object $instance, object num);
    var op_$mult7(object $instance, object num);
    var op_$mult8(object $instance, object num);
    var op_$mult9(object $instance, object num);
    var op_$mult10(object $instance, object num);
    var op_$mult11(object $instance, var& num);
    var op_$div(object $instance, object num);
    var op_$div2(object $instance, object num);
    var op_$div3(object $instance, object num);
    var op_$div4(object $instance, object num);
    var op_$div5(object $instance, object num);
    var op_$div6(object $instance, object num);
    var op_$div7(object $instance, object num);
    var op_$div8(object $instance, object num);
    var op_$div9(object $instance, object num);
    var op_$div10(object $instance, object num);
    var op_$div11(object $instance, var& num);
    var op_$mod(object $instance, object num);
    var op_$mod2(object $instance, object num);
    var op_$mod3(object $instance, object num);
    var op_$mod4(object $instance, object num);
    var op_$mod5(object $instance, object num);
    var op_$mod6(object $instance, object num);
    var op_$mod7(object $instance, object num);
    var op_$mod8(object $instance, object num);
    var op_$mod9(object $instance, object num);
    var op_$mod10(object $instance, object num);
    var op_$mod11(object $instance, var& num);
    var op_$less_than(object $instance, object num);
    var op_$less_than2(object $instance, object num);
    var op_$less_than3(object $instance, object num);
    var op_$less_than4(object $instance, object num);
    var op_$less_than5(object $instance, object num);
    var op_$less_than6(object $instance, object num);
    var op_$less_than7(object $instance, object num);
    var op_$less_than8(object $instance, object num);
    var op_$less_than9(object $instance, object num);
    var op_$less_than10(object $instance, object num);
    var op_$less_than11(object $instance, var& num);
    var op_$greater_than(object $instance, object num);
    var op_$greater_than2(object $instance, object num);
    var op_$greater_than3(object $instance, object num);
    var op_$greater_than4(object $instance, object num);
    var op_$greater_than5(object $instance, object num);
    var op_$greater_than6(object $instance, object num);
    var op_$greater_than7(object $instance, object num);
    var op_$greater_than8(object $instance, object num);
    var op_$greater_than9(object $instance, object num);
    var op_$greater_than10(object $instance, object num);
    var op_$greater_than11(object $instance, var& num);
    var op_$less_or_equals(object $instance, object num);
    var op_$less_or_equals2(object $instance, object num);
    var op_$less_or_equals3(object $instance, object num);
    var op_$less_or_equals4(object $instance, object num);
    var op_$less_or_equals5(object $instance, object num);
    var op_$less_or_equals6(object $instance, object num);
    var op_$less_or_equals7(object $instance, object num);
    var op_$less_or_equals8(object $instance, object num);
    var op_$less_or_equals9(object $instance, object num);
    var op_$less_or_equals10(object $instance, object num);
    var op_$less_or_equals11(object $instance, var& num);
    var op_$pow(object $instance, object num);
    var op_$pow2(object $instance, object num);
    var op_$pow3(object $instance, object num);
    var op_$pow4(object $instance, object num);
    var op_$pow5(object $instance, object num);
    var op_$pow6(object $instance, object num);
    var op_$pow7(object $instance, object num);
    var op_$pow8(object $instance, object num);
    var op_$pow9(object $instance, object num);
    var op_$pow10(object $instance, object num);
    var op_$pow11(object $instance, var& num);
    var op_$left_shift(object $instance, object num);
    var op_$left_shift2(object $instance, object num);
    var op_$left_shift3(object $instance, object num);
    var op_$left_shift4(object $instance, object num);
    var op_$left_shift5(object $instance, object num);
    var op_$left_shift6(object $instance, object num);
    var op_$left_shift7(object $instance, object num);
    var op_$left_shift8(object $instance, object num);
    var op_$left_shift9(object $instance, object num);
    var op_$left_shift10(object $instance, object num);
    var op_$left_shift11(object $instance, var& num);
    var op_$right_shift(object $instance, object num);
    var op_$right_shift2(object $instance, object num);
    var op_$right_shift3(object $instance, object num);
    var op_$right_shift4(object $instance, object num);
    var op_$right_shift5(object $instance, object num);
    var op_$right_shift6(object $instance, object num);
    var op_$right_shift7(object $instance, object num);
    var op_$right_shift8(object $instance, object num);
    var op_$right_shift9(object $instance, object num);
    var op_$right_shift10(object $instance, object num);
    var op_$right_shift11(object $instance, var& num);
    var op_$xor(object $instance, object num);
    var op_$xor2(object $instance, object num);
    var op_$xor3(object $instance, object num);
    var op_$xor4(object $instance, object num);
    var op_$xor5(object $instance, object num);
    var op_$xor6(object $instance, object num);
    var op_$xor7(object $instance, object num);
    var op_$xor8(object $instance, object num);
    var op_$xor9(object $instance, object num);
    var op_$xor10(object $instance, object num);
    var op_$xor11(object $instance, var& num);
    var op_$and(object $instance, object num);
    var op_$and2(object $instance, object num);
    var op_$and3(object $instance, object num);
    var op_$and4(object $instance, object num);
    var op_$and5(object $instance, object num);
    var op_$and6(object $instance, object num);
    var op_$and7(object $instance, object num);
    var op_$and8(object $instance, object num);
    var op_$and9(object $instance, object num);
    var op_$and10(object $instance, object num);
    var op_$and11(object $instance, var& num);
    var op_$or(object $instance, object num);
    var op_$or2(object $instance, object num);
    var op_$or3(object $instance, object num);
    var op_$or4(object $instance, object num);
    var op_$or5(object $instance, object num);
    var op_$or6(object $instance, object num);
    var op_$or7(object $instance, object num);
    var op_$or8(object $instance, object num);
    var op_$or9(object $instance, object num);
    var op_$or10(object $instance, object num);
    var op_$or11(object $instance, var& num);
    var op_$not_equals11(object $instance, var& num);
    var op_$equals_equals11(object $instance, var& num);
    _uint64 op_$equals(object $instance, var& num);
    object to_string(object $instance);
    _uint64 parse(object s);
    object to_ulong(object $instance);
    object to_long(object $instance);
    object to_uint(object $instance);
    object to_int(object $instance);
    object to_ushort(object $instance);
    object to_short(object $instance);
    object to_uchar(object $instance);
    object to_char(object $instance);
    object to_byte(object $instance);
    object to_bool(object $instance);
    _uint64 get_value(object $instance);
    void integer2(object $instance);
scope_end()

scope_begin(std, integer$_int64$)

    void integer(object $instance, var& initial_val);
    object hash(object $instance);
    var op_$not_equals(object $instance, object num);
    var op_$equals_equals(object $instance, object num);
    var op_$not_equals2(object $instance, object num);
    var op_$equals_equals2(object $instance, object num);
    var op_$not_equals3(object $instance, object num);
    var op_$equals_equals3(object $instance, object num);
    var op_$not_equals4(object $instance, object num);
    var op_$equals_equals4(object $instance, object num);
    var op_$not_equals5(object $instance, object num);
    var op_$equals_equals5(object $instance, object num);
    var op_$not_equals6(object $instance, object num);
    var op_$equals_equals6(object $instance, object num);
    var op_$not_equals7(object $instance, object num);
    var op_$equals_equals7(object $instance, object num);
    var op_$not_equals8(object $instance, object num);
    var op_$equals_equals8(object $instance, object num);
    var op_$not_equals9(object $instance, object num);
    var op_$equals_equals9(object $instance, object num);
    var op_$not_equals10(object $instance, object num);
    var op_$equals_equals10(object $instance, object num);
    var op_$plus(object $instance, object num);
    var op_$plus2(object $instance, object num);
    var op_$plus3(object $instance, object num);
    var op_$plus4(object $instance, object num);
    var op_$plus5(object $instance, object num);
    var op_$plus6(object $instance, object num);
    var op_$plus7(object $instance, object num);
    var op_$plus8(object $instance, object num);
    var op_$plus9(object $instance, object num);
    var op_$plus10(object $instance, object num);
    var op_$plus11(object $instance, var& num);
    var op_$minus(object $instance, object num);
    var op_$minus2(object $instance, object num);
    var op_$minus3(object $instance, object num);
    var op_$minus4(object $instance, object num);
    var op_$minus5(object $instance, object num);
    var op_$minus6(object $instance, object num);
    var op_$minus7(object $instance, object num);
    var op_$minus8(object $instance, object num);
    var op_$minus9(object $instance, object num);
    var op_$minus10(object $instance, object num);
    var op_$minus11(object $instance, var& num);
    var op_$mult(object $instance, object num);
    var op_$mult2(object $instance, object num);
    var op_$mult3(object $instance, object num);
    var op_$mult4(object $instance, object num);
    var op_$mult5(object $instance, object num);
    var op_$mult6(object $instance, object num);
    var op_$mult7(object $instance, object num);
    var op_$mult8(object $instance, object num);
    var op_$mult9(object $instance, object num);
    var op_$mult10(object $instance, object num);
    var op_$mult11(object $instance, var& num);
    var op_$div(object $instance, object num);
    var op_$div2(object $instance, object num);
    var op_$div3(object $instance, object num);
    var op_$div4(object $instance, object num);
    var op_$div5(object $instance, object num);
    var op_$div6(object $instance, object num);
    var op_$div7(object $instance, object num);
    var op_$div8(object $instance, object num);
    var op_$div9(object $instance, object num);
    var op_$div10(object $instance, object num);
    var op_$div11(object $instance, var& num);
    var op_$mod(object $instance, object num);
    var op_$mod2(object $instance, object num);
    var op_$mod3(object $instance, object num);
    var op_$mod4(object $instance, object num);
    var op_$mod5(object $instance, object num);
    var op_$mod6(object $instance, object num);
    var op_$mod7(object $instance, object num);
    var op_$mod8(object $instance, object num);
    var op_$mod9(object $instance, object num);
    var op_$mod10(object $instance, object num);
    var op_$mod11(object $instance, var& num);
    var op_$less_than(object $instance, object num);
    var op_$less_than2(object $instance, object num);
    var op_$less_than3(object $instance, object num);
    var op_$less_than4(object $instance, object num);
    var op_$less_than5(object $instance, object num);
    var op_$less_than6(object $instance, object num);
    var op_$less_than7(object $instance, object num);
    var op_$less_than8(object $instance, object num);
    var op_$less_than9(object $instance, object num);
    var op_$less_than10(object $instance, object num);
    var op_$less_than11(object $instance, var& num);
    var op_$greater_than(object $instance, object num);
    var op_$greater_than2(object $instance, object num);
    var op_$greater_than3(object $instance, object num);
    var op_$greater_than4(object $instance, object num);
    var op_$greater_than5(object $instance, object num);
    var op_$greater_than6(object $instance, object num);
    var op_$greater_than7(object $instance, object num);
    var op_$greater_than8(object $instance, object num);
    var op_$greater_than9(object $instance, object num);
    var op_$greater_than10(object $instance, object num);
    var op_$greater_than11(object $instance, var& num);
    var op_$less_or_equals(object $instance, object num);
    var op_$less_or_equals2(object $instance, object num);
    var op_$less_or_equals3(object $instance, object num);
    var op_$less_or_equals4(object $instance, object num);
    var op_$less_or_equals5(object $instance, object num);
    var op_$less_or_equals6(object $instance, object num);
    var op_$less_or_equals7(object $instance, object num);
    var op_$less_or_equals8(object $instance, object num);
    var op_$less_or_equals9(object $instance, object num);
    var op_$less_or_equals10(object $instance, object num);
    var op_$less_or_equals11(object $instance, var& num);
    var op_$pow(object $instance, object num);
    var op_$pow2(object $instance, object num);
    var op_$pow3(object $instance, object num);
    var op_$pow4(object $instance, object num);
    var op_$pow5(object $instance, object num);
    var op_$pow6(object $instance, object num);
    var op_$pow7(object $instance, object num);
    var op_$pow8(object $instance, object num);
    var op_$pow9(object $instance, object num);
    var op_$pow10(object $instance, object num);
    var op_$pow11(object $instance, var& num);
    var op_$left_shift(object $instance, object num);
    var op_$left_shift2(object $instance, object num);
    var op_$left_shift3(object $instance, object num);
    var op_$left_shift4(object $instance, object num);
    var op_$left_shift5(object $instance, object num);
    var op_$left_shift6(object $instance, object num);
    var op_$left_shift7(object $instance, object num);
    var op_$left_shift8(object $instance, object num);
    var op_$left_shift9(object $instance, object num);
    var op_$left_shift10(object $instance, object num);
    var op_$left_shift11(object $instance, var& num);
    var op_$right_shift(object $instance, object num);
    var op_$right_shift2(object $instance, object num);
    var op_$right_shift3(object $instance, object num);
    var op_$right_shift4(object $instance, object num);
    var op_$right_shift5(object $instance, object num);
    var op_$right_shift6(object $instance, object num);
    var op_$right_shift7(object $instance, object num);
    var op_$right_shift8(object $instance, object num);
    var op_$right_shift9(object $instance, object num);
    var op_$right_shift10(object $instance, object num);
    var op_$right_shift11(object $instance, var& num);
    var op_$xor(object $instance, object num);
    var op_$xor2(object $instance, object num);
    var op_$xor3(object $instance, object num);
    var op_$xor4(object $instance, object num);
    var op_$xor5(object $instance, object num);
    var op_$xor6(object $instance, object num);
    var op_$xor7(object $instance, object num);
    var op_$xor8(object $instance, object num);
    var op_$xor9(object $instance, object num);
    var op_$xor10(object $instance, object num);
    var op_$xor11(object $instance, var& num);
    var op_$and(object $instance, object num);
    var op_$and2(object $instance, object num);
    var op_$and3(object $instance, object num);
    var op_$and4(object $instance, object num);
    var op_$and5(object $instance, object num);
    var op_$and6(object $instance, object num);
    var op_$and7(object $instance, object num);
    var op_$and8(object $instance, object num);
    var op_$and9(object $instance, object num);
    var op_$and10(object $instance, object num);
    var op_$and11(object $instance, var& num);
    var op_$or(object $instance, object num);
    var op_$or2(object $instance, object num);
    var op_$or3(object $instance, object num);
    var op_$or4(object $instance, object num);
    var op_$or5(object $instance, object num);
    var op_$or6(object $instance, object num);
    var op_$or7(object $instance, object num);
    var op_$or8(object $instance, object num);
    var op_$or9(object $instance, object num);
    var op_$or10(object $instance, object num);
    var op_$or11(object $instance, var& num);
    var op_$not_equals11(object $instance, var& num);
    var op_$equals_equals11(object $instance, var& num);
    _int64 op_$equals(object $instance, var& num);
    object to_string(object $instance);
    _int64 parse(object s);
    object to_ulong(object $instance);
    object to_long(object $instance);
    object to_uint(object $instance);
    object to_int(object $instance);
    object to_ushort(object $instance);
    object to_short(object $instance);
    object to_uchar(object $instance);
    object to_char(object $instance);
    object to_byte(object $instance);
    object to_bool(object $instance);
    _int64 get_value(object $instance);
    void integer2(object $instance);
scope_end()

scope_begin(std, integer$_uint32$)

    void integer(object $instance, var& initial_val);
    object hash(object $instance);
    var op_$not_equals(object $instance, object num);
    var op_$equals_equals(object $instance, object num);
    var op_$not_equals2(object $instance, object num);
    var op_$equals_equals2(object $instance, object num);
    var op_$not_equals3(object $instance, object num);
    var op_$equals_equals3(object $instance, object num);
    var op_$not_equals4(object $instance, object num);
    var op_$equals_equals4(object $instance, object num);
    var op_$not_equals5(object $instance, object num);
    var op_$equals_equals5(object $instance, object num);
    var op_$not_equals6(object $instance, object num);
    var op_$equals_equals6(object $instance, object num);
    var op_$not_equals7(object $instance, object num);
    var op_$equals_equals7(object $instance, object num);
    var op_$not_equals8(object $instance, object num);
    var op_$equals_equals8(object $instance, object num);
    var op_$not_equals9(object $instance, object num);
    var op_$equals_equals9(object $instance, object num);
    var op_$not_equals10(object $instance, object num);
    var op_$equals_equals10(object $instance, object num);
    var op_$plus(object $instance, object num);
    var op_$plus2(object $instance, object num);
    var op_$plus3(object $instance, object num);
    var op_$plus4(object $instance, object num);
    var op_$plus5(object $instance, object num);
    var op_$plus6(object $instance, object num);
    var op_$plus7(object $instance, object num);
    var op_$plus8(object $instance, object num);
    var op_$plus9(object $instance, object num);
    var op_$plus10(object $instance, object num);
    var op_$plus11(object $instance, var& num);
    var op_$minus(object $instance, object num);
    var op_$minus2(object $instance, object num);
    var op_$minus3(object $instance, object num);
    var op_$minus4(object $instance, object num);
    var op_$minus5(object $instance, object num);
    var op_$minus6(object $instance, object num);
    var op_$minus7(object $instance, object num);
    var op_$minus8(object $instance, object num);
    var op_$minus9(object $instance, object num);
    var op_$minus10(object $instance, object num);
    var op_$minus11(object $instance, var& num);
    var op_$mult(object $instance, object num);
    var op_$mult2(object $instance, object num);
    var op_$mult3(object $instance, object num);
    var op_$mult4(object $instance, object num);
    var op_$mult5(object $instance, object num);
    var op_$mult6(object $instance, object num);
    var op_$mult7(object $instance, object num);
    var op_$mult8(object $instance, object num);
    var op_$mult9(object $instance, object num);
    var op_$mult10(object $instance, object num);
    var op_$mult11(object $instance, var& num);
    var op_$div(object $instance, object num);
    var op_$div2(object $instance, object num);
    var op_$div3(object $instance, object num);
    var op_$div4(object $instance, object num);
    var op_$div5(object $instance, object num);
    var op_$div6(object $instance, object num);
    var op_$div7(object $instance, object num);
    var op_$div8(object $instance, object num);
    var op_$div9(object $instance, object num);
    var op_$div10(object $instance, object num);
    var op_$div11(object $instance, var& num);
    var op_$mod(object $instance, object num);
    var op_$mod2(object $instance, object num);
    var op_$mod3(object $instance, object num);
    var op_$mod4(object $instance, object num);
    var op_$mod5(object $instance, object num);
    var op_$mod6(object $instance, object num);
    var op_$mod7(object $instance, object num);
    var op_$mod8(object $instance, object num);
    var op_$mod9(object $instance, object num);
    var op_$mod10(object $instance, object num);
    var op_$mod11(object $instance, var& num);
    var op_$less_than(object $instance, object num);
    var op_$less_than2(object $instance, object num);
    var op_$less_than3(object $instance, object num);
    var op_$less_than4(object $instance, object num);
    var op_$less_than5(object $instance, object num);
    var op_$less_than6(object $instance, object num);
    var op_$less_than7(object $instance, object num);
    var op_$less_than8(object $instance, object num);
    var op_$less_than9(object $instance, object num);
    var op_$less_than10(object $instance, object num);
    var op_$less_than11(object $instance, var& num);
    var op_$greater_than(object $instance, object num);
    var op_$greater_than2(object $instance, object num);
    var op_$greater_than3(object $instance, object num);
    var op_$greater_than4(object $instance, object num);
    var op_$greater_than5(object $instance, object num);
    var op_$greater_than6(object $instance, object num);
    var op_$greater_than7(object $instance, object num);
    var op_$greater_than8(object $instance, object num);
    var op_$greater_than9(object $instance, object num);
    var op_$greater_than10(object $instance, object num);
    var op_$greater_than11(object $instance, var& num);
    var op_$less_or_equals(object $instance, object num);
    var op_$less_or_equals2(object $instance, object num);
    var op_$less_or_equals3(object $instance, object num);
    var op_$less_or_equals4(object $instance, object num);
    var op_$less_or_equals5(object $instance, object num);
    var op_$less_or_equals6(object $instance, object num);
    var op_$less_or_equals7(object $instance, object num);
    var op_$less_or_equals8(object $instance, object num);
    var op_$less_or_equals9(object $instance, object num);
    var op_$less_or_equals10(object $instance, object num);
    var op_$less_or_equals11(object $instance, var& num);
    var op_$pow(object $instance, object num);
    var op_$pow2(object $instance, object num);
    var op_$pow3(object $instance, object num);
    var op_$pow4(object $instance, object num);
    var op_$pow5(object $instance, object num);
    var op_$pow6(object $instance, object num);
    var op_$pow7(object $instance, object num);
    var op_$pow8(object $instance, object num);
    var op_$pow9(object $instance, object num);
    var op_$pow10(object $instance, object num);
    var op_$pow11(object $instance, var& num);
    var op_$left_shift(object $instance, object num);
    var op_$left_shift2(object $instance, object num);
    var op_$left_shift3(object $instance, object num);
    var op_$left_shift4(object $instance, object num);
    var op_$left_shift5(object $instance, object num);
    var op_$left_shift6(object $instance, object num);
    var op_$left_shift7(object $instance, object num);
    var op_$left_shift8(object $instance, object num);
    var op_$left_shift9(object $instance, object num);
    var op_$left_shift10(object $instance, object num);
    var op_$left_shift11(object $instance, var& num);
    var op_$right_shift(object $instance, object num);
    var op_$right_shift2(object $instance, object num);
    var op_$right_shift3(object $instance, object num);
    var op_$right_shift4(object $instance, object num);
    var op_$right_shift5(object $instance, object num);
    var op_$right_shift6(object $instance, object num);
    var op_$right_shift7(object $instance, object num);
    var op_$right_shift8(object $instance, object num);
    var op_$right_shift9(object $instance, object num);
    var op_$right_shift10(object $instance, object num);
    var op_$right_shift11(object $instance, var& num);
    var op_$xor(object $instance, object num);
    var op_$xor2(object $instance, object num);
    var op_$xor3(object $instance, object num);
    var op_$xor4(object $instance, object num);
    var op_$xor5(object $instance, object num);
    var op_$xor6(object $instance, object num);
    var op_$xor7(object $instance, object num);
    var op_$xor8(object $instance, object num);
    var op_$xor9(object $instance, object num);
    var op_$xor10(object $instance, object num);
    var op_$xor11(object $instance, var& num);
    var op_$and(object $instance, object num);
    var op_$and2(object $instance, object num);
    var op_$and3(object $instance, object num);
    var op_$and4(object $instance, object num);
    var op_$and5(object $instance, object num);
    var op_$and6(object $instance, object num);
    var op_$and7(object $instance, object num);
    var op_$and8(object $instance, object num);
    var op_$and9(object $instance, object num);
    var op_$and10(object $instance, object num);
    var op_$and11(object $instance, var& num);
    var op_$or(object $instance, object num);
    var op_$or2(object $instance, object num);
    var op_$or3(object $instance, object num);
    var op_$or4(object $instance, object num);
    var op_$or5(object $instance, object num);
    var op_$or6(object $instance, object num);
    var op_$or7(object $instance, object num);
    var op_$or8(object $instance, object num);
    var op_$or9(object $instance, object num);
    var op_$or10(object $instance, object num);
    var op_$or11(object $instance, var& num);
    var op_$not_equals11(object $instance, var& num);
    var op_$equals_equals11(object $instance, var& num);
    _uint32 op_$equals(object $instance, var& num);
    object to_string(object $instance);
    _uint32 parse(object s);
    object to_ulong(object $instance);
    object to_long(object $instance);
    object to_uint(object $instance);
    object to_int(object $instance);
    object to_ushort(object $instance);
    object to_short(object $instance);
    object to_uchar(object $instance);
    object to_char(object $instance);
    object to_byte(object $instance);
    object to_bool(object $instance);
    _uint32 get_value(object $instance);
    void integer2(object $instance);
scope_end()

scope_begin(std, integer$_int32$)

    void integer(object $instance, var& initial_val);
    object hash(object $instance);
    var op_$not_equals(object $instance, object num);
    var op_$equals_equals(object $instance, object num);
    var op_$not_equals2(object $instance, object num);
    var op_$equals_equals2(object $instance, object num);
    var op_$not_equals3(object $instance, object num);
    var op_$equals_equals3(object $instance, object num);
    var op_$not_equals4(object $instance, object num);
    var op_$equals_equals4(object $instance, object num);
    var op_$not_equals5(object $instance, object num);
    var op_$equals_equals5(object $instance, object num);
    var op_$not_equals6(object $instance, object num);
    var op_$equals_equals6(object $instance, object num);
    var op_$not_equals7(object $instance, object num);
    var op_$equals_equals7(object $instance, object num);
    var op_$not_equals8(object $instance, object num);
    var op_$equals_equals8(object $instance, object num);
    var op_$not_equals9(object $instance, object num);
    var op_$equals_equals9(object $instance, object num);
    var op_$not_equals10(object $instance, object num);
    var op_$equals_equals10(object $instance, object num);
    var op_$plus(object $instance, object num);
    var op_$plus2(object $instance, object num);
    var op_$plus3(object $instance, object num);
    var op_$plus4(object $instance, object num);
    var op_$plus5(object $instance, object num);
    var op_$plus6(object $instance, object num);
    var op_$plus7(object $instance, object num);
    var op_$plus8(object $instance, object num);
    var op_$plus9(object $instance, object num);
    var op_$plus10(object $instance, object num);
    var op_$plus11(object $instance, var& num);
    var op_$minus(object $instance, object num);
    var op_$minus2(object $instance, object num);
    var op_$minus3(object $instance, object num);
    var op_$minus4(object $instance, object num);
    var op_$minus5(object $instance, object num);
    var op_$minus6(object $instance, object num);
    var op_$minus7(object $instance, object num);
    var op_$minus8(object $instance, object num);
    var op_$minus9(object $instance, object num);
    var op_$minus10(object $instance, object num);
    var op_$minus11(object $instance, var& num);
    var op_$mult(object $instance, object num);
    var op_$mult2(object $instance, object num);
    var op_$mult3(object $instance, object num);
    var op_$mult4(object $instance, object num);
    var op_$mult5(object $instance, object num);
    var op_$mult6(object $instance, object num);
    var op_$mult7(object $instance, object num);
    var op_$mult8(object $instance, object num);
    var op_$mult9(object $instance, object num);
    var op_$mult10(object $instance, object num);
    var op_$mult11(object $instance, var& num);
    var op_$div(object $instance, object num);
    var op_$div2(object $instance, object num);
    var op_$div3(object $instance, object num);
    var op_$div4(object $instance, object num);
    var op_$div5(object $instance, object num);
    var op_$div6(object $instance, object num);
    var op_$div7(object $instance, object num);
    var op_$div8(object $instance, object num);
    var op_$div9(object $instance, object num);
    var op_$div10(object $instance, object num);
    var op_$div11(object $instance, var& num);
    var op_$mod(object $instance, object num);
    var op_$mod2(object $instance, object num);
    var op_$mod3(object $instance, object num);
    var op_$mod4(object $instance, object num);
    var op_$mod5(object $instance, object num);
    var op_$mod6(object $instance, object num);
    var op_$mod7(object $instance, object num);
    var op_$mod8(object $instance, object num);
    var op_$mod9(object $instance, object num);
    var op_$mod10(object $instance, object num);
    var op_$mod11(object $instance, var& num);
    var op_$less_than(object $instance, object num);
    var op_$less_than2(object $instance, object num);
    var op_$less_than3(object $instance, object num);
    var op_$less_than4(object $instance, object num);
    var op_$less_than5(object $instance, object num);
    var op_$less_than6(object $instance, object num);
    var op_$less_than7(object $instance, object num);
    var op_$less_than8(object $instance, object num);
    var op_$less_than9(object $instance, object num);
    var op_$less_than10(object $instance, object num);
    var op_$less_than11(object $instance, var& num);
    var op_$greater_than(object $instance, object num);
    var op_$greater_than2(object $instance, object num);
    var op_$greater_than3(object $instance, object num);
    var op_$greater_than4(object $instance, object num);
    var op_$greater_than5(object $instance, object num);
    var op_$greater_than6(object $instance, object num);
    var op_$greater_than7(object $instance, object num);
    var op_$greater_than8(object $instance, object num);
    var op_$greater_than9(object $instance, object num);
    var op_$greater_than10(object $instance, object num);
    var op_$greater_than11(object $instance, var& num);
    var op_$less_or_equals(object $instance, object num);
    var op_$less_or_equals2(object $instance, object num);
    var op_$less_or_equals3(object $instance, object num);
    var op_$less_or_equals4(object $instance, object num);
    var op_$less_or_equals5(object $instance, object num);
    var op_$less_or_equals6(object $instance, object num);
    var op_$less_or_equals7(object $instance, object num);
    var op_$less_or_equals8(object $instance, object num);
    var op_$less_or_equals9(object $instance, object num);
    var op_$less_or_equals10(object $instance, object num);
    var op_$less_or_equals11(object $instance, var& num);
    var op_$pow(object $instance, object num);
    var op_$pow2(object $instance, object num);
    var op_$pow3(object $instance, object num);
    var op_$pow4(object $instance, object num);
    var op_$pow5(object $instance, object num);
    var op_$pow6(object $instance, object num);
    var op_$pow7(object $instance, object num);
    var op_$pow8(object $instance, object num);
    var op_$pow9(object $instance, object num);
    var op_$pow10(object $instance, object num);
    var op_$pow11(object $instance, var& num);
    var op_$left_shift(object $instance, object num);
    var op_$left_shift2(object $instance, object num);
    var op_$left_shift3(object $instance, object num);
    var op_$left_shift4(object $instance, object num);
    var op_$left_shift5(object $instance, object num);
    var op_$left_shift6(object $instance, object num);
    var op_$left_shift7(object $instance, object num);
    var op_$left_shift8(object $instance, object num);
    var op_$left_shift9(object $instance, object num);
    var op_$left_shift10(object $instance, object num);
    var op_$left_shift11(object $instance, var& num);
    var op_$right_shift(object $instance, object num);
    var op_$right_shift2(object $instance, object num);
    var op_$right_shift3(object $instance, object num);
    var op_$right_shift4(object $instance, object num);
    var op_$right_shift5(object $instance, object num);
    var op_$right_shift6(object $instance, object num);
    var op_$right_shift7(object $instance, object num);
    var op_$right_shift8(object $instance, object num);
    var op_$right_shift9(object $instance, object num);
    var op_$right_shift10(object $instance, object num);
    var op_$right_shift11(object $instance, var& num);
    var op_$xor(object $instance, object num);
    var op_$xor2(object $instance, object num);
    var op_$xor3(object $instance, object num);
    var op_$xor4(object $instance, object num);
    var op_$xor5(object $instance, object num);
    var op_$xor6(object $instance, object num);
    var op_$xor7(object $instance, object num);
    var op_$xor8(object $instance, object num);
    var op_$xor9(object $instance, object num);
    var op_$xor10(object $instance, object num);
    var op_$xor11(object $instance, var& num);
    var op_$and(object $instance, object num);
    var op_$and2(object $instance, object num);
    var op_$and3(object $instance, object num);
    var op_$and4(object $instance, object num);
    var op_$and5(object $instance, object num);
    var op_$and6(object $instance, object num);
    var op_$and7(object $instance, object num);
    var op_$and8(object $instance, object num);
    var op_$and9(object $instance, object num);
    var op_$and10(object $instance, object num);
    var op_$and11(object $instance, var& num);
    var op_$or(object $instance, object num);
    var op_$or2(object $instance, object num);
    var op_$or3(object $instance, object num);
    var op_$or4(object $instance, object num);
    var op_$or5(object $instance, object num);
    var op_$or6(object $instance, object num);
    var op_$or7(object $instance, object num);
    var op_$or8(object $instance, object num);
    var op_$or9(object $instance, object num);
    var op_$or10(object $instance, object num);
    var op_$or11(object $instance, var& num);
    var op_$not_equals11(object $instance, var& num);
    var op_$equals_equals11(object $instance, var& num);
    _int32 op_$equals(object $instance, var& num);
    object to_string(object $instance);
    _int32 parse(object s);
    object to_ulong(object $instance);
    object to_long(object $instance);
    object to_uint(object $instance);
    object to_int(object $instance);
    object to_ushort(object $instance);
    object to_short(object $instance);
    object to_uchar(object $instance);
    object to_char(object $instance);
    object to_byte(object $instance);
    object to_bool(object $instance);
    _int32 get_value(object $instance);
    void integer2(object $instance);
scope_end()

scope_begin(std, integer$_uint16$)

    void integer(object $instance, var& initial_val);
    object hash(object $instance);
    var op_$not_equals(object $instance, object num);
    var op_$equals_equals(object $instance, object num);
    var op_$not_equals2(object $instance, object num);
    var op_$equals_equals2(object $instance, object num);
    var op_$not_equals3(object $instance, object num);
    var op_$equals_equals3(object $instance, object num);
    var op_$not_equals4(object $instance, object num);
    var op_$equals_equals4(object $instance, object num);
    var op_$not_equals5(object $instance, object num);
    var op_$equals_equals5(object $instance, object num);
    var op_$not_equals6(object $instance, object num);
    var op_$equals_equals6(object $instance, object num);
    var op_$not_equals7(object $instance, object num);
    var op_$equals_equals7(object $instance, object num);
    var op_$not_equals8(object $instance, object num);
    var op_$equals_equals8(object $instance, object num);
    var op_$not_equals9(object $instance, object num);
    var op_$equals_equals9(object $instance, object num);
    var op_$not_equals10(object $instance, object num);
    var op_$equals_equals10(object $instance, object num);
    var op_$plus(object $instance, object num);
    var op_$plus2(object $instance, object num);
    var op_$plus3(object $instance, object num);
    var op_$plus4(object $instance, object num);
    var op_$plus5(object $instance, object num);
    var op_$plus6(object $instance, object num);
    var op_$plus7(object $instance, object num);
    var op_$plus8(object $instance, object num);
    var op_$plus9(object $instance, object num);
    var op_$plus10(object $instance, object num);
    var op_$plus11(object $instance, var& num);
    var op_$minus(object $instance, object num);
    var op_$minus2(object $instance, object num);
    var op_$minus3(object $instance, object num);
    var op_$minus4(object $instance, object num);
    var op_$minus5(object $instance, object num);
    var op_$minus6(object $instance, object num);
    var op_$minus7(object $instance, object num);
    var op_$minus8(object $instance, object num);
    var op_$minus9(object $instance, object num);
    var op_$minus10(object $instance, object num);
    var op_$minus11(object $instance, var& num);
    var op_$mult(object $instance, object num);
    var op_$mult2(object $instance, object num);
    var op_$mult3(object $instance, object num);
    var op_$mult4(object $instance, object num);
    var op_$mult5(object $instance, object num);
    var op_$mult6(object $instance, object num);
    var op_$mult7(object $instance, object num);
    var op_$mult8(object $instance, object num);
    var op_$mult9(object $instance, object num);
    var op_$mult10(object $instance, object num);
    var op_$mult11(object $instance, var& num);
    var op_$div(object $instance, object num);
    var op_$div2(object $instance, object num);
    var op_$div3(object $instance, object num);
    var op_$div4(object $instance, object num);
    var op_$div5(object $instance, object num);
    var op_$div6(object $instance, object num);
    var op_$div7(object $instance, object num);
    var op_$div8(object $instance, object num);
    var op_$div9(object $instance, object num);
    var op_$div10(object $instance, object num);
    var op_$div11(object $instance, var& num);
    var op_$mod(object $instance, object num);
    var op_$mod2(object $instance, object num);
    var op_$mod3(object $instance, object num);
    var op_$mod4(object $instance, object num);
    var op_$mod5(object $instance, object num);
    var op_$mod6(object $instance, object num);
    var op_$mod7(object $instance, object num);
    var op_$mod8(object $instance, object num);
    var op_$mod9(object $instance, object num);
    var op_$mod10(object $instance, object num);
    var op_$mod11(object $instance, var& num);
    var op_$less_than(object $instance, object num);
    var op_$less_than2(object $instance, object num);
    var op_$less_than3(object $instance, object num);
    var op_$less_than4(object $instance, object num);
    var op_$less_than5(object $instance, object num);
    var op_$less_than6(object $instance, object num);
    var op_$less_than7(object $instance, object num);
    var op_$less_than8(object $instance, object num);
    var op_$less_than9(object $instance, object num);
    var op_$less_than10(object $instance, object num);
    var op_$less_than11(object $instance, var& num);
    var op_$greater_than(object $instance, object num);
    var op_$greater_than2(object $instance, object num);
    var op_$greater_than3(object $instance, object num);
    var op_$greater_than4(object $instance, object num);
    var op_$greater_than5(object $instance, object num);
    var op_$greater_than6(object $instance, object num);
    var op_$greater_than7(object $instance, object num);
    var op_$greater_than8(object $instance, object num);
    var op_$greater_than9(object $instance, object num);
    var op_$greater_than10(object $instance, object num);
    var op_$greater_than11(object $instance, var& num);
    var op_$less_or_equals(object $instance, object num);
    var op_$less_or_equals2(object $instance, object num);
    var op_$less_or_equals3(object $instance, object num);
    var op_$less_or_equals4(object $instance, object num);
    var op_$less_or_equals5(object $instance, object num);
    var op_$less_or_equals6(object $instance, object num);
    var op_$less_or_equals7(object $instance, object num);
    var op_$less_or_equals8(object $instance, object num);
    var op_$less_or_equals9(object $instance, object num);
    var op_$less_or_equals10(object $instance, object num);
    var op_$less_or_equals11(object $instance, var& num);
    var op_$pow(object $instance, object num);
    var op_$pow2(object $instance, object num);
    var op_$pow3(object $instance, object num);
    var op_$pow4(object $instance, object num);
    var op_$pow5(object $instance, object num);
    var op_$pow6(object $instance, object num);
    var op_$pow7(object $instance, object num);
    var op_$pow8(object $instance, object num);
    var op_$pow9(object $instance, object num);
    var op_$pow10(object $instance, object num);
    var op_$pow11(object $instance, var& num);
    var op_$left_shift(object $instance, object num);
    var op_$left_shift2(object $instance, object num);
    var op_$left_shift3(object $instance, object num);
    var op_$left_shift4(object $instance, object num);
    var op_$left_shift5(object $instance, object num);
    var op_$left_shift6(object $instance, object num);
    var op_$left_shift7(object $instance, object num);
    var op_$left_shift8(object $instance, object num);
    var op_$left_shift9(object $instance, object num);
    var op_$left_shift10(object $instance, object num);
    var op_$left_shift11(object $instance, var& num);
    var op_$right_shift(object $instance, object num);
    var op_$right_shift2(object $instance, object num);
    var op_$right_shift3(object $instance, object num);
    var op_$right_shift4(object $instance, object num);
    var op_$right_shift5(object $instance, object num);
    var op_$right_shift6(object $instance, object num);
    var op_$right_shift7(object $instance, object num);
    var op_$right_shift8(object $instance, object num);
    var op_$right_shift9(object $instance, object num);
    var op_$right_shift10(object $instance, object num);
    var op_$right_shift11(object $instance, var& num);
    var op_$xor(object $instance, object num);
    var op_$xor2(object $instance, object num);
    var op_$xor3(object $instance, object num);
    var op_$xor4(object $instance, object num);
    var op_$xor5(object $instance, object num);
    var op_$xor6(object $instance, object num);
    var op_$xor7(object $instance, object num);
    var op_$xor8(object $instance, object num);
    var op_$xor9(object $instance, object num);
    var op_$xor10(object $instance, object num);
    var op_$xor11(object $instance, var& num);
    var op_$and(object $instance, object num);
    var op_$and2(object $instance, object num);
    var op_$and3(object $instance, object num);
    var op_$and4(object $instance, object num);
    var op_$and5(object $instance, object num);
    var op_$and6(object $instance, object num);
    var op_$and7(object $instance, object num);
    var op_$and8(object $instance, object num);
    var op_$and9(object $instance, object num);
    var op_$and10(object $instance, object num);
    var op_$and11(object $instance, var& num);
    var op_$or(object $instance, object num);
    var op_$or2(object $instance, object num);
    var op_$or3(object $instance, object num);
    var op_$or4(object $instance, object num);
    var op_$or5(object $instance, object num);
    var op_$or6(object $instance, object num);
    var op_$or7(object $instance, object num);
    var op_$or8(object $instance, object num);
    var op_$or9(object $instance, object num);
    var op_$or10(object $instance, object num);
    var op_$or11(object $instance, var& num);
    var op_$not_equals11(object $instance, var& num);
    var op_$equals_equals11(object $instance, var& num);
    _uint16 op_$equals(object $instance, var& num);
    object to_string(object $instance);
    _uint16 parse(object s);
    object to_ulong(object $instance);
    object to_long(object $instance);
    object to_uint(object $instance);
    object to_int(object $instance);
    object to_ushort(object $instance);
    object to_short(object $instance);
    object to_uchar(object $instance);
    object to_char(object $instance);
    object to_byte(object $instance);
    object to_bool(object $instance);
    _uint16 get_value(object $instance);
    void integer2(object $instance);
scope_end()

scope_begin(std, integer$_int16$)

    void integer(object $instance, var& initial_val);
    object hash(object $instance);
    var op_$not_equals(object $instance, object num);
    var op_$equals_equals(object $instance, object num);
    var op_$not_equals2(object $instance, object num);
    var op_$equals_equals2(object $instance, object num);
    var op_$not_equals3(object $instance, object num);
    var op_$equals_equals3(object $instance, object num);
    var op_$not_equals4(object $instance, object num);
    var op_$equals_equals4(object $instance, object num);
    var op_$not_equals5(object $instance, object num);
    var op_$equals_equals5(object $instance, object num);
    var op_$not_equals6(object $instance, object num);
    var op_$equals_equals6(object $instance, object num);
    var op_$not_equals7(object $instance, object num);
    var op_$equals_equals7(object $instance, object num);
    var op_$not_equals8(object $instance, object num);
    var op_$equals_equals8(object $instance, object num);
    var op_$not_equals9(object $instance, object num);
    var op_$equals_equals9(object $instance, object num);
    var op_$not_equals10(object $instance, object num);
    var op_$equals_equals10(object $instance, object num);
    var op_$plus(object $instance, object num);
    var op_$plus2(object $instance, object num);
    var op_$plus3(object $instance, object num);
    var op_$plus4(object $instance, object num);
    var op_$plus5(object $instance, object num);
    var op_$plus6(object $instance, object num);
    var op_$plus7(object $instance, object num);
    var op_$plus8(object $instance, object num);
    var op_$plus9(object $instance, object num);
    var op_$plus10(object $instance, object num);
    var op_$plus11(object $instance, var& num);
    var op_$minus(object $instance, object num);
    var op_$minus2(object $instance, object num);
    var op_$minus3(object $instance, object num);
    var op_$minus4(object $instance, object num);
    var op_$minus5(object $instance, object num);
    var op_$minus6(object $instance, object num);
    var op_$minus7(object $instance, object num);
    var op_$minus8(object $instance, object num);
    var op_$minus9(object $instance, object num);
    var op_$minus10(object $instance, object num);
    var op_$minus11(object $instance, var& num);
    var op_$mult(object $instance, object num);
    var op_$mult2(object $instance, object num);
    var op_$mult3(object $instance, object num);
    var op_$mult4(object $instance, object num);
    var op_$mult5(object $instance, object num);
    var op_$mult6(object $instance, object num);
    var op_$mult7(object $instance, object num);
    var op_$mult8(object $instance, object num);
    var op_$mult9(object $instance, object num);
    var op_$mult10(object $instance, object num);
    var op_$mult11(object $instance, var& num);
    var op_$div(object $instance, object num);
    var op_$div2(object $instance, object num);
    var op_$div3(object $instance, object num);
    var op_$div4(object $instance, object num);
    var op_$div5(object $instance, object num);
    var op_$div6(object $instance, object num);
    var op_$div7(object $instance, object num);
    var op_$div8(object $instance, object num);
    var op_$div9(object $instance, object num);
    var op_$div10(object $instance, object num);
    var op_$div11(object $instance, var& num);
    var op_$mod(object $instance, object num);
    var op_$mod2(object $instance, object num);
    var op_$mod3(object $instance, object num);
    var op_$mod4(object $instance, object num);
    var op_$mod5(object $instance, object num);
    var op_$mod6(object $instance, object num);
    var op_$mod7(object $instance, object num);
    var op_$mod8(object $instance, object num);
    var op_$mod9(object $instance, object num);
    var op_$mod10(object $instance, object num);
    var op_$mod11(object $instance, var& num);
    var op_$less_than(object $instance, object num);
    var op_$less_than2(object $instance, object num);
    var op_$less_than3(object $instance, object num);
    var op_$less_than4(object $instance, object num);
    var op_$less_than5(object $instance, object num);
    var op_$less_than6(object $instance, object num);
    var op_$less_than7(object $instance, object num);
    var op_$less_than8(object $instance, object num);
    var op_$less_than9(object $instance, object num);
    var op_$less_than10(object $instance, object num);
    var op_$less_than11(object $instance, var& num);
    var op_$greater_than(object $instance, object num);
    var op_$greater_than2(object $instance, object num);
    var op_$greater_than3(object $instance, object num);
    var op_$greater_than4(object $instance, object num);
    var op_$greater_than5(object $instance, object num);
    var op_$greater_than6(object $instance, object num);
    var op_$greater_than7(object $instance, object num);
    var op_$greater_than8(object $instance, object num);
    var op_$greater_than9(object $instance, object num);
    var op_$greater_than10(object $instance, object num);
    var op_$greater_than11(object $instance, var& num);
    var op_$less_or_equals(object $instance, object num);
    var op_$less_or_equals2(object $instance, object num);
    var op_$less_or_equals3(object $instance, object num);
    var op_$less_or_equals4(object $instance, object num);
    var op_$less_or_equals5(object $instance, object num);
    var op_$less_or_equals6(object $instance, object num);
    var op_$less_or_equals7(object $instance, object num);
    var op_$less_or_equals8(object $instance, object num);
    var op_$less_or_equals9(object $instance, object num);
    var op_$less_or_equals10(object $instance, object num);
    var op_$less_or_equals11(object $instance, var& num);
    var op_$pow(object $instance, object num);
    var op_$pow2(object $instance, object num);
    var op_$pow3(object $instance, object num);
    var op_$pow4(object $instance, object num);
    var op_$pow5(object $instance, object num);
    var op_$pow6(object $instance, object num);
    var op_$pow7(object $instance, object num);
    var op_$pow8(object $instance, object num);
    var op_$pow9(object $instance, object num);
    var op_$pow10(object $instance, object num);
    var op_$pow11(object $instance, var& num);
    var op_$left_shift(object $instance, object num);
    var op_$left_shift2(object $instance, object num);
    var op_$left_shift3(object $instance, object num);
    var op_$left_shift4(object $instance, object num);
    var op_$left_shift5(object $instance, object num);
    var op_$left_shift6(object $instance, object num);
    var op_$left_shift7(object $instance, object num);
    var op_$left_shift8(object $instance, object num);
    var op_$left_shift9(object $instance, object num);
    var op_$left_shift10(object $instance, object num);
    var op_$left_shift11(object $instance, var& num);
    var op_$right_shift(object $instance, object num);
    var op_$right_shift2(object $instance, object num);
    var op_$right_shift3(object $instance, object num);
    var op_$right_shift4(object $instance, object num);
    var op_$right_shift5(object $instance, object num);
    var op_$right_shift6(object $instance, object num);
    var op_$right_shift7(object $instance, object num);
    var op_$right_shift8(object $instance, object num);
    var op_$right_shift9(object $instance, object num);
    var op_$right_shift10(object $instance, object num);
    var op_$right_shift11(object $instance, var& num);
    var op_$xor(object $instance, object num);
    var op_$xor2(object $instance, object num);
    var op_$xor3(object $instance, object num);
    var op_$xor4(object $instance, object num);
    var op_$xor5(object $instance, object num);
    var op_$xor6(object $instance, object num);
    var op_$xor7(object $instance, object num);
    var op_$xor8(object $instance, object num);
    var op_$xor9(object $instance, object num);
    var op_$xor10(object $instance, object num);
    var op_$xor11(object $instance, var& num);
    var op_$and(object $instance, object num);
    var op_$and2(object $instance, object num);
    var op_$and3(object $instance, object num);
    var op_$and4(object $instance, object num);
    var op_$and5(object $instance, object num);
    var op_$and6(object $instance, object num);
    var op_$and7(object $instance, object num);
    var op_$and8(object $instance, object num);
    var op_$and9(object $instance, object num);
    var op_$and10(object $instance, object num);
    var op_$and11(object $instance, var& num);
    var op_$or(object $instance, object num);
    var op_$or2(object $instance, object num);
    var op_$or3(object $instance, object num);
    var op_$or4(object $instance, object num);
    var op_$or5(object $instance, object num);
    var op_$or6(object $instance, object num);
    var op_$or7(object $instance, object num);
    var op_$or8(object $instance, object num);
    var op_$or9(object $instance, object num);
    var op_$or10(object $instance, object num);
    var op_$or11(object $instance, var& num);
    var op_$not_equals11(object $instance, var& num);
    var op_$equals_equals11(object $instance, var& num);
    _int16 op_$equals(object $instance, var& num);
    object to_string(object $instance);
    _int16 parse(object s);
    object to_ulong(object $instance);
    object to_long(object $instance);
    object to_uint(object $instance);
    object to_int(object $instance);
    object to_ushort(object $instance);
    object to_short(object $instance);
    object to_uchar(object $instance);
    object to_char(object $instance);
    object to_byte(object $instance);
    object to_bool(object $instance);
    _int16 get_value(object $instance);
    void integer2(object $instance);
scope_end()

scope_begin(std, integer$_uint8$)

    void integer(object $instance, var& initial_val);
    object hash(object $instance);
    var op_$not_equals(object $instance, object num);
    var op_$equals_equals(object $instance, object num);
    var op_$not_equals2(object $instance, object num);
    var op_$equals_equals2(object $instance, object num);
    var op_$not_equals3(object $instance, object num);
    var op_$equals_equals3(object $instance, object num);
    var op_$not_equals4(object $instance, object num);
    var op_$equals_equals4(object $instance, object num);
    var op_$not_equals5(object $instance, object num);
    var op_$equals_equals5(object $instance, object num);
    var op_$not_equals6(object $instance, object num);
    var op_$equals_equals6(object $instance, object num);
    var op_$not_equals7(object $instance, object num);
    var op_$equals_equals7(object $instance, object num);
    var op_$not_equals8(object $instance, object num);
    var op_$equals_equals8(object $instance, object num);
    var op_$not_equals9(object $instance, object num);
    var op_$equals_equals9(object $instance, object num);
    var op_$not_equals10(object $instance, object num);
    var op_$equals_equals10(object $instance, object num);
    var op_$plus(object $instance, object num);
    var op_$plus2(object $instance, object num);
    var op_$plus3(object $instance, object num);
    var op_$plus4(object $instance, object num);
    var op_$plus5(object $instance, object num);
    var op_$plus6(object $instance, object num);
    var op_$plus7(object $instance, object num);
    var op_$plus8(object $instance, object num);
    var op_$plus9(object $instance, object num);
    var op_$plus10(object $instance, object num);
    var op_$plus11(object $instance, var& num);
    var op_$minus(object $instance, object num);
    var op_$minus2(object $instance, object num);
    var op_$minus3(object $instance, object num);
    var op_$minus4(object $instance, object num);
    var op_$minus5(object $instance, object num);
    var op_$minus6(object $instance, object num);
    var op_$minus7(object $instance, object num);
    var op_$minus8(object $instance, object num);
    var op_$minus9(object $instance, object num);
    var op_$minus10(object $instance, object num);
    var op_$minus11(object $instance, var& num);
    var op_$mult(object $instance, object num);
    var op_$mult2(object $instance, object num);
    var op_$mult3(object $instance, object num);
    var op_$mult4(object $instance, object num);
    var op_$mult5(object $instance, object num);
    var op_$mult6(object $instance, object num);
    var op_$mult7(object $instance, object num);
    var op_$mult8(object $instance, object num);
    var op_$mult9(object $instance, object num);
    var op_$mult10(object $instance, object num);
    var op_$mult11(object $instance, var& num);
    var op_$div(object $instance, object num);
    var op_$div2(object $instance, object num);
    var op_$div3(object $instance, object num);
    var op_$div4(object $instance, object num);
    var op_$div5(object $instance, object num);
    var op_$div6(object $instance, object num);
    var op_$div7(object $instance, object num);
    var op_$div8(object $instance, object num);
    var op_$div9(object $instance, object num);
    var op_$div10(object $instance, object num);
    var op_$div11(object $instance, var& num);
    var op_$mod(object $instance, object num);
    var op_$mod2(object $instance, object num);
    var op_$mod3(object $instance, object num);
    var op_$mod4(object $instance, object num);
    var op_$mod5(object $instance, object num);
    var op_$mod6(object $instance, object num);
    var op_$mod7(object $instance, object num);
    var op_$mod8(object $instance, object num);
    var op_$mod9(object $instance, object num);
    var op_$mod10(object $instance, object num);
    var op_$mod11(object $instance, var& num);
    var op_$less_than(object $instance, object num);
    var op_$less_than2(object $instance, object num);
    var op_$less_than3(object $instance, object num);
    var op_$less_than4(object $instance, object num);
    var op_$less_than5(object $instance, object num);
    var op_$less_than6(object $instance, object num);
    var op_$less_than7(object $instance, object num);
    var op_$less_than8(object $instance, object num);
    var op_$less_than9(object $instance, object num);
    var op_$less_than10(object $instance, object num);
    var op_$less_than11(object $instance, var& num);
    var op_$greater_than(object $instance, object num);
    var op_$greater_than2(object $instance, object num);
    var op_$greater_than3(object $instance, object num);
    var op_$greater_than4(object $instance, object num);
    var op_$greater_than5(object $instance, object num);
    var op_$greater_than6(object $instance, object num);
    var op_$greater_than7(object $instance, object num);
    var op_$greater_than8(object $instance, object num);
    var op_$greater_than9(object $instance, object num);
    var op_$greater_than10(object $instance, object num);
    var op_$greater_than11(object $instance, var& num);
    var op_$less_or_equals(object $instance, object num);
    var op_$less_or_equals2(object $instance, object num);
    var op_$less_or_equals3(object $instance, object num);
    var op_$less_or_equals4(object $instance, object num);
    var op_$less_or_equals5(object $instance, object num);
    var op_$less_or_equals6(object $instance, object num);
    var op_$less_or_equals7(object $instance, object num);
    var op_$less_or_equals8(object $instance, object num);
    var op_$less_or_equals9(object $instance, object num);
    var op_$less_or_equals10(object $instance, object num);
    var op_$less_or_equals11(object $instance, var& num);
    var op_$pow(object $instance, object num);
    var op_$pow2(object $instance, object num);
    var op_$pow3(object $instance, object num);
    var op_$pow4(object $instance, object num);
    var op_$pow5(object $instance, object num);
    var op_$pow6(object $instance, object num);
    var op_$pow7(object $instance, object num);
    var op_$pow8(object $instance, object num);
    var op_$pow9(object $instance, object num);
    var op_$pow10(object $instance, object num);
    var op_$pow11(object $instance, var& num);
    var op_$left_shift(object $instance, object num);
    var op_$left_shift2(object $instance, object num);
    var op_$left_shift3(object $instance, object num);
    var op_$left_shift4(object $instance, object num);
    var op_$left_shift5(object $instance, object num);
    var op_$left_shift6(object $instance, object num);
    var op_$left_shift7(object $instance, object num);
    var op_$left_shift8(object $instance, object num);
    var op_$left_shift9(object $instance, object num);
    var op_$left_shift10(object $instance, object num);
    var op_$left_shift11(object $instance, var& num);
    var op_$right_shift(object $instance, object num);
    var op_$right_shift2(object $instance, object num);
    var op_$right_shift3(object $instance, object num);
    var op_$right_shift4(object $instance, object num);
    var op_$right_shift5(object $instance, object num);
    var op_$right_shift6(object $instance, object num);
    var op_$right_shift7(object $instance, object num);
    var op_$right_shift8(object $instance, object num);
    var op_$right_shift9(object $instance, object num);
    var op_$right_shift10(object $instance, object num);
    var op_$right_shift11(object $instance, var& num);
    var op_$xor(object $instance, object num);
    var op_$xor2(object $instance, object num);
    var op_$xor3(object $instance, object num);
    var op_$xor4(object $instance, object num);
    var op_$xor5(object $instance, object num);
    var op_$xor6(object $instance, object num);
    var op_$xor7(object $instance, object num);
    var op_$xor8(object $instance, object num);
    var op_$xor9(object $instance, object num);
    var op_$xor10(object $instance, object num);
    var op_$xor11(object $instance, var& num);
    var op_$and(object $instance, object num);
    var op_$and2(object $instance, object num);
    var op_$and3(object $instance, object num);
    var op_$and4(object $instance, object num);
    var op_$and5(object $instance, object num);
    var op_$and6(object $instance, object num);
    var op_$and7(object $instance, object num);
    var op_$and8(object $instance, object num);
    var op_$and9(object $instance, object num);
    var op_$and10(object $instance, object num);
    var op_$and11(object $instance, var& num);
    var op_$or(object $instance, object num);
    var op_$or2(object $instance, object num);
    var op_$or3(object $instance, object num);
    var op_$or4(object $instance, object num);
    var op_$or5(object $instance, object num);
    var op_$or6(object $instance, object num);
    var op_$or7(object $instance, object num);
    var op_$or8(object $instance, object num);
    var op_$or9(object $instance, object num);
    var op_$or10(object $instance, object num);
    var op_$or11(object $instance, var& num);
    var op_$not_equals11(object $instance, var& num);
    var op_$equals_equals11(object $instance, var& num);
    _uint8 op_$equals(object $instance, var& num);
    object to_string(object $instance);
    _uint8 parse(object s);
    object to_ulong(object $instance);
    object to_long(object $instance);
    object to_uint(object $instance);
    object to_int(object $instance);
    object to_ushort(object $instance);
    object to_short(object $instance);
    object to_uchar(object $instance);
    object to_char(object $instance);
    object to_byte(object $instance);
    object to_bool(object $instance);
    _uint8 get_value(object $instance);
    void integer2(object $instance);
scope_end()

scope_begin(std, integer$_int8$)

    void integer(object $instance, var& initial_val);
    object hash(object $instance);
    var op_$not_equals(object $instance, object num);
    var op_$equals_equals(object $instance, object num);
    var op_$not_equals2(object $instance, object num);
    var op_$equals_equals2(object $instance, object num);
    var op_$not_equals3(object $instance, object num);
    var op_$equals_equals3(object $instance, object num);
    var op_$not_equals4(object $instance, object num);
    var op_$equals_equals4(object $instance, object num);
    var op_$not_equals5(object $instance, object num);
    var op_$equals_equals5(object $instance, object num);
    var op_$not_equals6(object $instance, object num);
    var op_$equals_equals6(object $instance, object num);
    var op_$not_equals7(object $instance, object num);
    var op_$equals_equals7(object $instance, object num);
    var op_$not_equals8(object $instance, object num);
    var op_$equals_equals8(object $instance, object num);
    var op_$not_equals9(object $instance, object num);
    var op_$equals_equals9(object $instance, object num);
    var op_$not_equals10(object $instance, object num);
    var op_$equals_equals10(object $instance, object num);
    var op_$plus(object $instance, object num);
    var op_$plus2(object $instance, object num);
    var op_$plus3(object $instance, object num);
    var op_$plus4(object $instance, object num);
    var op_$plus5(object $instance, object num);
    var op_$plus6(object $instance, object num);
    var op_$plus7(object $instance, object num);
    var op_$plus8(object $instance, object num);
    var op_$plus9(object $instance, object num);
    var op_$plus10(object $instance, object num);
    var op_$plus11(object $instance, var& num);
    var op_$minus(object $instance, object num);
    var op_$minus2(object $instance, object num);
    var op_$minus3(object $instance, object num);
    var op_$minus4(object $instance, object num);
    var op_$minus5(object $instance, object num);
    var op_$minus6(object $instance, object num);
    var op_$minus7(object $instance, object num);
    var op_$minus8(object $instance, object num);
    var op_$minus9(object $instance, object num);
    var op_$minus10(object $instance, object num);
    var op_$minus11(object $instance, var& num);
    var op_$mult(object $instance, object num);
    var op_$mult2(object $instance, object num);
    var op_$mult3(object $instance, object num);
    var op_$mult4(object $instance, object num);
    var op_$mult5(object $instance, object num);
    var op_$mult6(object $instance, object num);
    var op_$mult7(object $instance, object num);
    var op_$mult8(object $instance, object num);
    var op_$mult9(object $instance, object num);
    var op_$mult10(object $instance, object num);
    var op_$mult11(object $instance, var& num);
    var op_$div(object $instance, object num);
    var op_$div2(object $instance, object num);
    var op_$div3(object $instance, object num);
    var op_$div4(object $instance, object num);
    var op_$div5(object $instance, object num);
    var op_$div6(object $instance, object num);
    var op_$div7(object $instance, object num);
    var op_$div8(object $instance, object num);
    var op_$div9(object $instance, object num);
    var op_$div10(object $instance, object num);
    var op_$div11(object $instance, var& num);
    var op_$mod(object $instance, object num);
    var op_$mod2(object $instance, object num);
    var op_$mod3(object $instance, object num);
    var op_$mod4(object $instance, object num);
    var op_$mod5(object $instance, object num);
    var op_$mod6(object $instance, object num);
    var op_$mod7(object $instance, object num);
    var op_$mod8(object $instance, object num);
    var op_$mod9(object $instance, object num);
    var op_$mod10(object $instance, object num);
    var op_$mod11(object $instance, var& num);
    var op_$less_than(object $instance, object num);
    var op_$less_than2(object $instance, object num);
    var op_$less_than3(object $instance, object num);
    var op_$less_than4(object $instance, object num);
    var op_$less_than5(object $instance, object num);
    var op_$less_than6(object $instance, object num);
    var op_$less_than7(object $instance, object num);
    var op_$less_than8(object $instance, object num);
    var op_$less_than9(object $instance, object num);
    var op_$less_than10(object $instance, object num);
    var op_$less_than11(object $instance, var& num);
    var op_$greater_than(object $instance, object num);
    var op_$greater_than2(object $instance, object num);
    var op_$greater_than3(object $instance, object num);
    var op_$greater_than4(object $instance, object num);
    var op_$greater_than5(object $instance, object num);
    var op_$greater_than6(object $instance, object num);
    var op_$greater_than7(object $instance, object num);
    var op_$greater_than8(object $instance, object num);
    var op_$greater_than9(object $instance, object num);
    var op_$greater_than10(object $instance, object num);
    var op_$greater_than11(object $instance, var& num);
    var op_$less_or_equals(object $instance, object num);
    var op_$less_or_equals2(object $instance, object num);
    var op_$less_or_equals3(object $instance, object num);
    var op_$less_or_equals4(object $instance, object num);
    var op_$less_or_equals5(object $instance, object num);
    var op_$less_or_equals6(object $instance, object num);
    var op_$less_or_equals7(object $instance, object num);
    var op_$less_or_equals8(object $instance, object num);
    var op_$less_or_equals9(object $instance, object num);
    var op_$less_or_equals10(object $instance, object num);
    var op_$less_or_equals11(object $instance, var& num);
    var op_$pow(object $instance, object num);
    var op_$pow2(object $instance, object num);
    var op_$pow3(object $instance, object num);
    var op_$pow4(object $instance, object num);
    var op_$pow5(object $instance, object num);
    var op_$pow6(object $instance, object num);
    var op_$pow7(object $instance, object num);
    var op_$pow8(object $instance, object num);
    var op_$pow9(object $instance, object num);
    var op_$pow10(object $instance, object num);
    var op_$pow11(object $instance, var& num);
    var op_$left_shift(object $instance, object num);
    var op_$left_shift2(object $instance, object num);
    var op_$left_shift3(object $instance, object num);
    var op_$left_shift4(object $instance, object num);
    var op_$left_shift5(object $instance, object num);
    var op_$left_shift6(object $instance, object num);
    var op_$left_shift7(object $instance, object num);
    var op_$left_shift8(object $instance, object num);
    var op_$left_shift9(object $instance, object num);
    var op_$left_shift10(object $instance, object num);
    var op_$left_shift11(object $instance, var& num);
    var op_$right_shift(object $instance, object num);
    var op_$right_shift2(object $instance, object num);
    var op_$right_shift3(object $instance, object num);
    var op_$right_shift4(object $instance, object num);
    var op_$right_shift5(object $instance, object num);
    var op_$right_shift6(object $instance, object num);
    var op_$right_shift7(object $instance, object num);
    var op_$right_shift8(object $instance, object num);
    var op_$right_shift9(object $instance, object num);
    var op_$right_shift10(object $instance, object num);
    var op_$right_shift11(object $instance, var& num);
    var op_$xor(object $instance, object num);
    var op_$xor2(object $instance, object num);
    var op_$xor3(object $instance, object num);
    var op_$xor4(object $instance, object num);
    var op_$xor5(object $instance, object num);
    var op_$xor6(object $instance, object num);
    var op_$xor7(object $instance, object num);
    var op_$xor8(object $instance, object num);
    var op_$xor9(object $instance, object num);
    var op_$xor10(object $instance, object num);
    var op_$xor11(object $instance, var& num);
    var op_$and(object $instance, object num);
    var op_$and2(object $instance, object num);
    var op_$and3(object $instance, object num);
    var op_$and4(object $instance, object num);
    var op_$and5(object $instance, object num);
    var op_$and6(object $instance, object num);
    var op_$and7(object $instance, object num);
    var op_$and8(object $instance, object num);
    var op_$and9(object $instance, object num);
    var op_$and10(object $instance, object num);
    var op_$and11(object $instance, var& num);
    var op_$or(object $instance, object num);
    var op_$or2(object $instance, object num);
    var op_$or3(object $instance, object num);
    var op_$or4(object $instance, object num);
    var op_$or5(object $instance, object num);
    var op_$or6(object $instance, object num);
    var op_$or7(object $instance, object num);
    var op_$or8(object $instance, object num);
    var op_$or9(object $instance, object num);
    var op_$or10(object $instance, object num);
    var op_$or11(object $instance, var& num);
    var op_$not_equals11(object $instance, var& num);
    var op_$equals_equals11(object $instance, var& num);
    _int8 op_$equals(object $instance, var& num);
    object to_string(object $instance);
    _int8 parse(object s);
    object to_ulong(object $instance);
    object to_long(object $instance);
    object to_uint(object $instance);
    object to_int(object $instance);
    object to_ushort(object $instance);
    object to_short(object $instance);
    object to_uchar(object $instance);
    object to_char(object $instance);
    object to_byte(object $instance);
    object to_bool(object $instance);
    _int8 get_value(object $instance);
    void integer2(object $instance);
scope_end()

scope_begin(std, loopable$_int8$)

    _int8_array get_elements(object $instance);
    void loopable(object $instance);
scope_end()

scope_begin(std, list$std_string$)

    void list(object $instance);
    void list2(object $instance, var& starting_capacity);
    void list3(object $instance, object initial_data);
    var empty(object $instance);
    void set_compact(object $instance, var& compact_array);
    object get_elements(object $instance);
    object op_$array_at(object $instance, var& index);
    object at(object $instance, var& index);
    object last(object $instance);
    object put(object $instance, var& index, object element);
    void expand(object $instance);
    void clear(object $instance);
    void add_all(object $instance, object lst);
    void add_all2(object $instance, object lst);
    void add(object $instance, object element);
    var indexof(object $instance, object element);
    void pop_back(object $instance);
    var size(object $instance);
    void insert(object $instance, var& index, object element);
    void remove(object $instance, object val);
    void remove2(object $instance, object val, var& compare_fun);
    void remove_at(object $instance, var& index);
    var indexof2(object $instance, object comparer, var& find_func);
    object to_string(object $instance);
    object out_of_bounds_msg(object $instance, var& index);
scope_end()

scope_begin(std, loopable$std_string$)

    object get_elements(object $instance);
    void loopable(object $instance);
scope_end()

scope_begin(std, hashtable$std_io_thread_0_std_io_task_job_controller$)

    void hashtable(object $instance, var& initialCapacity);
    void hashtable2(object $instance);
    void set_threshold(object $instance, var& threshold);
    var hash(object $instance, object key);
    void resize(object $instance);
    object at(object $instance, object key);
    var put(object $instance, object key, object value);
    var remove(object $instance, object key);
scope_end()

scope_begin(std, hashmap$std_int_0_std_io_thread$)

    void hashmap(object $instance, var& initialCapacity);
    void hashmap2(object $instance);
    void set_threshold(object $instance, var& threshold);
    var hash(object $instance, object key);
    void resize(object $instance);
    object at(object $instance, object key);
    var put(object $instance, object key, object value);
    var remove(object $instance, object key);
scope_end()

scope_begin(std, list$std_io_thread$)

    void list(object $instance);
    void list2(object $instance, var& starting_capacity);
    void list3(object $instance, object initial_data);
    var empty(object $instance);
    void set_compact(object $instance, var& compact_array);
    object get_elements(object $instance);
    object op_$array_at(object $instance, var& index);
    object at(object $instance, var& index);
    object last(object $instance);
    object put(object $instance, var& index, object element);
    void expand(object $instance);
    void clear(object $instance);
    void add_all(object $instance, object lst);
    void add_all2(object $instance, object lst);
    void add(object $instance, object element);
    var indexof(object $instance, object element);
    void pop_back(object $instance);
    var size(object $instance);
    void insert(object $instance, var& index, object element);
    void remove(object $instance, object val);
    void remove2(object $instance, object val, var& compare_fun);
    void remove_at(object $instance, var& index);
    var indexof2(object $instance, object comparer, var& find_func);
    object to_string(object $instance);
    object out_of_bounds_msg(object $instance, var& index);
scope_end()

scope_begin(std, synced_list$std_io_task_job$)

    void synced_list(object $instance);
    void synced_list2(object $instance, var& starting_capacity);
    void set_compact(object $instance, var& compact_array);
    object get_elements(object $instance);
    object op_$array_at(object $instance, var& index);
    object at(object $instance, var& index);
    object last(object $instance);
    object put(object $instance, var& index, object element);
    void expand(object $instance);
    void clear(object $instance);
    void add_all(object $instance, object lst);
    void add(object $instance, object element);
    var indexof(object $instance, object element);
    void pop_back(object $instance);
    var size(object $instance);
    var empty(object $instance);
    void insert(object $instance, var& index, object element);
    void remove(object $instance, object val);
    void remove2(object $instance, object val, var& compare_fun);
    void remove_at(object $instance, var& index);
    var indexof2(object $instance, object comparer, var& find_func);
    object to_string(object $instance);
    object out_of_bounds_msg(object $instance, var& index);
scope_end()

scope_begin(std, loopable$std_io_task_job$)

    object get_elements(object $instance);
    void loopable(object $instance);
scope_end()

scope_begin(std, list$std_io_task_job$)

    void list(object $instance);
    void list2(object $instance, var& starting_capacity);
    void list3(object $instance, object initial_data);
    var empty(object $instance);
    void set_compact(object $instance, var& compact_array);
    object get_elements(object $instance);
    object op_$array_at(object $instance, var& index);
    object at(object $instance, var& index);
    object last(object $instance);
    object put(object $instance, var& index, object element);
    void expand(object $instance);
    void clear(object $instance);
    void add_all(object $instance, object lst);
    void add_all2(object $instance, object lst);
    void add(object $instance, object element);
    var indexof(object $instance, object element);
    void pop_back(object $instance);
    var size(object $instance);
    void insert(object $instance, var& index, object element);
    void remove(object $instance, object val);
    void remove2(object $instance, object val, var& compare_fun);
    void remove_at(object $instance, var& index);
    var indexof2(object $instance, object comparer, var& find_func);
    object to_string(object $instance);
    object out_of_bounds_msg(object $instance, var& index);
scope_end()

scope_begin(std, list$var$)

    void list(object $instance);
    void list2(object $instance, var& starting_capacity);
    void list3(object $instance, var_array& initial_data);
    var empty(object $instance);
    void set_compact(object $instance, var& compact_array);
    var_array get_elements(object $instance);
    var op_$array_at(object $instance, var& index);
    var at(object $instance, var& index);
    var last(object $instance);
    var put(object $instance, var& index, var& element);
    void expand(object $instance);
    void clear(object $instance);
    void add_all(object $instance, object lst);
    void add_all2(object $instance, var_array& lst);
    void add(object $instance, var& element);
    var indexof(object $instance, var& element);
    void pop_back(object $instance);
    var size(object $instance);
    void insert(object $instance, var& index, var& element);
    void remove(object $instance, var& val);
    void remove2(object $instance, var& val, var& compare_fun);
    void remove_at(object $instance, var& index);
    var indexof2(object $instance, var& comparer, var& find_func);
    object to_string(object $instance);
    object out_of_bounds_msg(object $instance, var& index);
scope_end()

scope_begin(std, loopable$var$)

    var_array get_elements(object $instance);
    void loopable(object $instance);
scope_end()

scope_begin(std_io_task, entry$std_io_thread_0_std_io_task_job_controller$)

    void entry(object $instance, object key, object value);
    object to_string(object $instance);
    void entry2(object $instance);
scope_end()

scope_begin(std_io, entry$std_int_0_std_io_thread$)

    void entry(object $instance, object key, object value);
    object to_string(object $instance);
    void entry2(object $instance);
scope_end()

#endif //SHARP_NATIVE_MAPPING_H
