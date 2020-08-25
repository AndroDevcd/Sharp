#include "native_mapping.h"

using namespace snb_api::internal;

scope_begin(std) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(0);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var_array snprintf(var& fmt, var& num, var& precision) {
		pushNum(fmt.value());
		pushNum(num.value());
		pushNum(precision.value());
		call(1570);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	void print(_int8_array& data) {
		pushObj(data.handle);
		call(1571);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print2(object str) {
		pushObj(str);
		call(1572);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println(var_array& data) {
		pushObj(data.handle);
		call(1573);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println2(_int8_array& data) {
		pushObj(data.handle);
		call(1574);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println3(_int16_array& data) {
		pushObj(data.handle);
		call(1575);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println4(_int32_array& data) {
		pushObj(data.handle);
		call(1576);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println5(_int64_array& data) {
		pushObj(data.handle);
		call(1577);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println6(_uint8_array& data) {
		pushObj(data.handle);
		call(1578);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println7(_uint16_array& data) {
		pushObj(data.handle);
		call(1579);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println8(_uint32_array& data) {
		pushObj(data.handle);
		call(1580);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println9(_uint64_array& data) {
		pushObj(data.handle);
		call(1581);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println10(var& data) {
		pushNum(data.value());
		call(1582);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println11() {
		call(1583);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println12(object e) {
		pushObj(e);
		call(1584);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object read_line() {
		call(1585);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object read_passwd() {
		call(1586);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var read_char() {
		call(1587);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var read_raw_char() {
		call(1588);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void println13(object str) {
		pushObj(str);
		call(1589);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void println14(object o) {
		pushObj(o);
		call(1590);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print3(object o) {
		pushObj(o);
		call(1591);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print4(var& data) {
		pushNum(data.value());
		call(1592);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void flush() {
		call(1593);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var utc_mills_time() {
		call(1594);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var nano_time() {
		call(1595);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void exit() {
		call(1596);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var sizeOf(object data) {
		pushObj(data);
		call(1597);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var sizeOf2(object data) {
		pushObj(data);
		call(1598);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void print_chars(_int8_array& str) {
		pushObj(str.handle);
		call(1599);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_num(var& val) {
		pushNum(val.value());
		call(1600);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_char(_int8& val) {
		pushNum(val.value());
		call(1601);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_numbers(var_array& str) {
		pushObj(str.handle);
		call(1602);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_numbers2(_int16_array& str) {
		pushObj(str.handle);
		call(1603);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_numbers3(_int32_array& str) {
		pushObj(str.handle);
		call(1604);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_numbers4(_int64_array& str) {
		pushObj(str.handle);
		call(1605);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_numbers5(_uint8_array& str) {
		pushObj(str.handle);
		call(1606);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_numbers6(_uint16_array& str) {
		pushObj(str.handle);
		call(1607);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_numbers7(_uint32_array& str) {
		pushObj(str.handle);
		call(1608);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_numbers8(_uint64_array& str) {
		pushObj(str.handle);
		call(1609);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void print_chars2(_int16_array& str) {
		pushObj(str.handle);
		call(1610);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var read_ch(var& hide) {
		pushNum(hide.value());
		call(1611);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object runtime_error(object message) {
		pushObj(message);
		call(1668);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object runtime_error2() {
		call(1669);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void todo(object message) {
		pushObj(message);
		call(1670);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3517);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, _enum_) 

	void _enum_(object $instance, var& ord) {
		pushObj($instance);
		pushNum(ord.value());
		call(1612);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _enum_2(object $instance, object e) {
		pushObj($instance);
		pushObj(e);
		call(1613);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var op_$not_equals(object $instance, object e) {
		pushObj($instance);
		pushObj(e);
		call(1614);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1615);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object e) {
		pushObj($instance);
		pushObj(e);
		call(1616);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1617);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8_array get_name(object $instance) {
		pushObj($instance);
		call(1618);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int64 get_ordinal(object $instance) {
		pushObj($instance);
		call(1619);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(1620);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void _enum_3(object $instance) {
		pushObj($instance);
		call(1621);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, throwable) 

	void throwable(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1622);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void throwable2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1623);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_message(object $instance) {
		pushObj($instance);
		call(1624);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void throwable3(object $instance) {
		pushObj($instance);
		call(1625);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_stack_trace(object $instance) {
		pushObj($instance);
		call(3288);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, exception) 

	void exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1626);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1627);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void exception3(object $instance) {
		pushObj($instance);
		call(1628);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, illegal_argument_exception) 

	void illegal_argument_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1629);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void illegal_argument_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1630);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void illegal_argument_exception3(object $instance) {
		pushObj($instance);
		call(1631);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, illegal_state_exception) 

	void illegal_state_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1632);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void illegal_state_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1633);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void illegal_state_exception3(object $instance) {
		pushObj($instance);
		call(1634);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, out_of_bounds_exception) 

	void out_of_bounds_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1635);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void out_of_bounds_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1636);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void out_of_bounds_exception3(object $instance) {
		pushObj($instance);
		call(1637);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, io_exception) 

	void io_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1638);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void io_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1639);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void io_exception3(object $instance) {
		pushObj($instance);
		call(1640);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, invalid_operation_exception) 

	void invalid_operation_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1641);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void invalid_operation_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1642);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void invalid_operation_exception3(object $instance) {
		pushObj($instance);
		call(1643);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, nullptr_exception) 

	void nullptr_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1644);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void nullptr_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1645);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void nullptr_exception3(object $instance) {
		pushObj($instance);
		call(1646);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, runtime_exception) 

	void runtime_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1647);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void runtime_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1648);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void runtime_exception3(object $instance) {
		pushObj($instance);
		call(1649);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, stack_overflow_exception) 

	void stack_overflow_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1650);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void stack_overflow_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1651);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void stack_overflow_exception3(object $instance) {
		pushObj($instance);
		call(1652);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, thread_stack_exception) 

	void thread_stack_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1653);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread_stack_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1654);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread_stack_exception3(object $instance) {
		pushObj($instance);
		call(1655);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, class_cast_exception) 

	void class_cast_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1656);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void class_cast_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1657);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void class_cast_exception3(object $instance) {
		pushObj($instance);
		call(1658);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, out_of_memory_exception) 

	void out_of_memory_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1659);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void out_of_memory_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1660);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void out_of_memory_exception3(object $instance) {
		pushObj($instance);
		call(1661);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, not_implemented_error) 

	void not_implemented_error(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1662);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void not_implemented_error2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1663);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void not_implemented_error3(object $instance) {
		pushObj($instance);
		call(1664);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, unsatisfied_link_error) 

	void unsatisfied_link_error(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1665);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void unsatisfied_link_error2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1666);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void unsatisfied_link_error3(object $instance) {
		pushObj($instance);
		call(1667);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(1);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void critical_section(var& block) {
		pushNum(block.value());
		call(1671);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void critical_section2(var& block, object lock_obj) {
		pushNum(block.value());
		pushObj(lock_obj);
		call(1672);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc(object data, var& size) {
		pushObj(data);
		pushNum(size.value());
		call(1735);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc2(var_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1736);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc3(_int8_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1737);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc4(_int16_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1738);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc5(_int32_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1739);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc6(_int64_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1740);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc7(_uint8_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1741);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc8(_uint16_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1742);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc9(_uint32_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1743);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void realloc10(_uint64_array& data, var& size) {
		pushObj(data.handle);
		pushNum(size.value());
		call(1744);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy(object src, object dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src);
		pushObj(dest);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1745);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy2(var_array& src, var_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1746);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy3(_int8_array& src, _int8_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1747);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy4(_int16_array& src, _int16_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1748);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy5(_int32_array& src, _int32_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1749);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy6(_int64_array& src, _int64_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1750);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy7(_uint8_array& src, _uint8_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1751);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy8(_uint16_array& src, _uint16_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1752);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy9(_uint32_array& src, _uint32_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1753);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void memcopy10(_uint64_array& src, _uint64_array& dest, var& destStart, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushObj(dest.handle);
		pushNum(destStart.value());
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1754);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object copy(object src, var& srcStart, var& srcEnd) {
		pushObj(src);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1755);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var_array copy2(var_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1756);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int8_array copy3(_int8_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1757);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int16_array copy4(_int16_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1758);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int16_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int32_array copy5(_int32_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1759);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int32_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_uint8_array copy6(_uint8_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1760);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_uint8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_uint16_array copy7(_uint16_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1761);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_uint16_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_uint32_array copy8(_uint32_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1762);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_uint32_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_uint64_array copy9(_uint64_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1763);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_uint64_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int64_array copy10(_int64_array& src, var& srcStart, var& srcEnd) {
		pushObj(src.handle);
		pushNum(srcStart.value());
		pushNum(srcEnd.value());
		call(1764);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int64_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	object invert(object src, var& start, var& len) {
		pushObj(src);
		pushNum(start.value());
		pushNum(len.value());
		call(1765);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var_array invert2(var_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1766);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int8_array invert3(_int8_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1767);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int16_array invert4(_int16_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1768);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int16_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int32_array invert5(_int32_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1769);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int32_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int64_array invert6(_int64_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1770);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int64_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_uint8_array invert7(_uint8_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1771);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_uint8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_uint16_array invert8(_uint16_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1772);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_uint16_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_uint32_array invert9(_uint32_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1773);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_uint32_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_uint64_array invert10(_uint64_array& src, var& start, var& len) {
		pushObj(src.handle);
		pushNum(start.value());
		pushNum(len.value());
		call(1774);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_uint64_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	void _srt_thread_start() {
		call(1890);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3511);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io, file) 

	void file(object $instance, object path) {
		pushObj($instance);
		pushObj(path);
		call(1673);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void file2(object $instance, _int8_array& path) {
		pushObj($instance);
		pushObj(path.handle);
		call(1674);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_name(object $instance) {
		pushObj($instance);
		call(1675);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(1676);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void update_path(object $instance, object path) {
		pushObj($instance);
		pushObj(path);
		call(1677);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_parent_file(object $instance) {
		pushObj($instance);
		call(1678);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object get_path(object $instance) {
		pushObj($instance);
		call(1679);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object get_absolute_path(object $instance) {
		pushObj($instance);
		call(1680);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var is_readable(object $instance) {
		pushObj($instance);
		call(1681);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_writeable(object $instance) {
		pushObj($instance);
		call(1682);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var exists(object $instance) {
		pushObj($instance);
		call(1683);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_directory(object $instance) {
		pushObj($instance);
		call(1684);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_file(object $instance) {
		pushObj($instance);
		call(1685);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_hidden(object $instance) {
		pushObj($instance);
		call(1686);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var last_modified(object $instance) {
		pushObj($instance);
		call(1687);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var last_status_changed(object $instance) {
		pushObj($instance);
		call(1688);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var last_accessed(object $instance) {
		pushObj($instance);
		call(1689);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var length(object $instance) {
		pushObj($instance);
		call(1690);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void create(object $instance) {
		pushObj($instance);
		call(1691);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var _delete(object $instance) {
		pushObj($instance);
		call(1692);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object list(object $instance) {
		pushObj($instance);
		call(1693);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var mkdir(object $instance) {
		pushObj($instance);
		call(1694);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var rename(object $instance, object dest) {
		pushObj($instance);
		pushObj(dest);
		call(1695);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_last_modified(object $instance, var& time) {
		pushObj($instance);
		pushNum(time.value());
		call(1696);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_read_only(object $instance) {
		pushObj($instance);
		call(1697);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_writable(object $instance, var& writable, var& owner_only) {
		pushObj($instance);
		pushNum(writable.value());
		pushNum(owner_only.value());
		call(1698);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_writable2(object $instance, var& writable) {
		pushObj($instance);
		pushNum(writable.value());
		call(1699);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_readable(object $instance, var& readable, var& owner_only) {
		pushObj($instance);
		pushNum(readable.value());
		pushNum(owner_only.value());
		call(1700);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_readable2(object $instance, var& readable) {
		pushObj($instance);
		pushNum(readable.value());
		call(1701);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_executable(object $instance, var& executable, var& owner_only) {
		pushObj($instance);
		pushNum(executable.value());
		pushNum(owner_only.value());
		call(1702);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_executable2(object $instance, var& executable) {
		pushObj($instance);
		pushNum(executable.value());
		call(1703);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_executable(object $instance) {
		pushObj($instance);
		call(1704);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_total_space(object $instance) {
		pushObj($instance);
		call(1705);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_free_space(object $instance) {
		pushObj($instance);
		call(1706);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_usable_space(object $instance) {
		pushObj($instance);
		call(1707);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object generate_temp_file(object prefix, object suffix, object dir) {
		pushObj(prefix);
		pushObj(suffix);
		pushObj(dir);
		call(1708);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object create_temp_file(object prefix, object suffix, object directory) {
		pushObj(prefix);
		pushObj(suffix);
		pushObj(directory);
		call(1709);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object create_temp_file2(object prefix, object suffix) {
		pushObj(prefix);
		pushObj(suffix);
		call(1710);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object read_all_text(object $instance) {
		pushObj($instance);
		call(1711);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var write(object $instance, object data) {
		pushObj($instance);
		pushObj(data);
		call(1712);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object get_file_system() {
		call(1713);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void file3(object $instance) {
		pushObj($instance);
		call(1714);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3512);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io, file_system) 

	var path_separator(object $instance) {
		pushObj($instance);
		call(1715);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object normalize(object $instance, object path) {
		pushObj($instance);
		pushObj(path);
		call(1716);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object resolve(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1717);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var get_attributes(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1718);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var check_access(object $instance, object f, var& access) {
		pushObj($instance);
		pushObj(f);
		pushNum(access.value());
		call(1719);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_permission(object $instance, object f, var& access, var& enable, var& owneronly) {
		pushObj($instance);
		pushObj(f);
		pushNum(access.value());
		pushNum(enable.value());
		pushNum(owneronly.value());
		call(1720);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var last_modified_time(object $instance, object f, var& stat_id) {
		pushObj($instance);
		pushObj(f);
		pushNum(stat_id.value());
		call(1721);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_length(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1722);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void create_file(object $instance, object pathname) {
		pushObj($instance);
		pushObj(pathname);
		call(1723);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var delete_directory(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1724);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var _delete(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1725);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object list(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1726);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var create_directory(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1727);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var rename(object $instance, object f1, object f2) {
		pushObj($instance);
		pushObj(f1);
		pushObj(f2);
		call(1728);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object read(object f) {
		pushObj(f);
		call(1729);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var write(object f, object data) {
		pushObj(f);
		pushObj(data);
		call(1730);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_last_modified_time(object $instance, object f, var& time) {
		pushObj($instance);
		pushObj(f);
		pushNum(time.value());
		call(1731);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_read_only(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1732);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_space_available(var& request) {
		pushNum(request.value());
		call(1733);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void file_system(object $instance) {
		pushObj($instance);
		call(1734);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io_task) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(2);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void start_scheduler() {
		call(1821);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void calculate_max_threads() {
		call(1822);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_thread_max(var& max) {
		pushNum(max.value());
		call(1823);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var get_thread_max() {
		call(1824);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var scheduler_main(object args) {
		pushObj(args);
		call(1825);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object get_host(var& long_running) {
		pushNum(long_running.value());
		call(1826);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void kill_threads() {
		call(1827);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void execute_synchronous(object scheduled_job) {
		pushObj(scheduled_job);
		call(1828);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var execute_job(object scheduled_job) {
		pushObj(scheduled_job);
		call(1829);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void finish() {
		call(1830);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3514);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var anon_func$3529(object it, object it2) {
		pushObj(it);
		pushObj(it2);
		call(3529);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var anon_func$3530(object it, object it2) {
		pushObj(it);
		pushObj(it2);
		call(3530);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var anon_func$3531(object it, object it2) {
		pushObj(it);
		pushObj(it2);
		call(3531);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var anon_func$3532(object t) {
		pushObj(t);
		call(3532);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var anon_func$3533(object t) {
		pushObj(t);
		call(3533);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var anon_func$3534(object t) {
		pushObj(t);
		call(3534);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std_io_task, cancellation_exception) 

	void cancellation_exception(object $instance, _int8_array& message) {
		pushObj($instance);
		pushObj(message.handle);
		call(1775);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void cancellation_exception2(object $instance, object message) {
		pushObj($instance);
		pushObj(message);
		call(1776);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void cancellation_exception3(object $instance) {
		pushObj($instance);
		call(1777);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io_task, job) 

	void job(object $instance) {
		pushObj($instance);
		call(1778);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void job2(object $instance, object name) {
		pushObj($instance);
		pushObj(name);
		call(1779);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var execute_async(object $instance) {
		pushObj($instance);
		call(1780);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void cancel(object $instance) {
		pushObj($instance);
		call(1781);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void cancel_future(object $instance) {
		pushObj($instance);
		call(1782);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void join(object $instance) {
		pushObj($instance);
		call(1783);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_state(object $instance) {
		pushObj($instance);
		call(1784);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object get_name(object $instance) {
		pushObj($instance);
		call(1785);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var get_id(object $instance) {
		pushObj($instance);
		call(1786);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void op_$equals(object $instance, var& block) {
		pushObj($instance);
		pushNum(block.value());
		call(1787);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void op_$equals2(object $instance, var& block) {
		pushObj($instance);
		pushNum(block.value());
		call(1788);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void op_$equals3(object $instance, var& block) {
		pushObj($instance);
		pushNum(block.value());
		call(1789);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void op_$equals4(object $instance, var& block) {
		pushObj($instance);
		pushNum(block.value());
		call(1790);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var job_main(object args) {
		pushObj(args);
		call(1791);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void execute(object $instance) {
		pushObj($instance);
		call(1792);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var is_finished(object $instance) {
		pushObj($instance);
		call(1793);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_active(object $instance) {
		pushObj($instance);
		call(1794);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void update_timeout(object $instance, var& tm) {
		pushObj($instance);
		pushNum(tm.value());
		call(1795);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void update_interval(object $instance, var& tm, object interval) {
		pushObj($instance);
		pushNum(tm.value());
		pushObj(interval);
		call(1796);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void reschedule(object $instance) {
		pushObj($instance);
		call(1797);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3513);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io_task, job_builder) 

	void job_builder(object $instance) {
		pushObj($instance);
		call(1798);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void job_builder2(object $instance, object scheduled_job, object master) {
		pushObj($instance);
		pushObj(scheduled_job);
		pushObj(master);
		call(1799);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object with_timeout(object $instance, var& mills_time) {
		pushObj($instance);
		pushNum(mills_time.value());
		call(1800);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object schedule(object $instance, var& future) {
		pushObj($instance);
		pushNum(future.value());
		call(1801);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object recurring(object $instance, var& tm, object interval) {
		pushObj($instance);
		pushNum(tm.value());
		pushObj(interval);
		call(1802);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object with_name(object $instance, object job_name) {
		pushObj($instance);
		pushObj(job_name);
		call(1803);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object with_args(object $instance, object args) {
		pushObj($instance);
		pushObj(args);
		call(1804);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object block(object $instance) {
		pushObj($instance);
		call(1805);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object long_term(object $instance) {
		pushObj($instance);
		call(1806);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$equals(object $instance, var& main) {
		pushObj($instance);
		pushNum(main.value());
		call(1807);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$equals2(object $instance, var& main) {
		pushObj($instance);
		pushNum(main.value());
		call(1808);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$equals3(object $instance, var& main) {
		pushObj($instance);
		pushNum(main.value());
		call(1809);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$equals4(object $instance, var& main) {
		pushObj($instance);
		pushNum(main.value());
		call(1810);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object get_job(object $instance) {
		pushObj($instance);
		call(1811);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std_io_task, job_controller) 

	void job_controller(object $instance) {
		pushObj($instance);
		call(1812);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io_task, job_master) 

	void job_master(object $instance) {
		pushObj($instance);
		call(1813);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object op_$equals(object $instance, var& main) {
		pushObj($instance);
		pushNum(main.value());
		call(1814);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object create_job(object $instance) {
		pushObj($instance);
		call(1815);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object create_job2(object $instance, object name) {
		pushObj($instance);
		pushObj(name);
		call(1816);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void start_job(object $instance, object scheduled_job) {
		pushObj($instance);
		pushObj(scheduled_job);
		call(1817);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_host_controller(object $instance, object host) {
		pushObj($instance);
		pushObj(host);
		call(1818);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void cleanup_job(object $instance, object scheduled_job) {
		pushObj($instance);
		pushObj(scheduled_job);
		call(1819);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void cancel_jobs_by_name(object $instance, object name) {
		pushObj($instance);
		pushObj(name);
		call(1820);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io_task, task) 

	object with_timeout(var& mills_time) {
		pushNum(mills_time.value());
		call(1831);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object schedule(var& future) {
		pushNum(future.value());
		call(1832);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object with_name(object job_name) {
		pushObj(job_name);
		call(1833);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object with_args(object args) {
		pushObj(args);
		call(1834);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object block() {
		call(1835);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object long_term() {
		call(1836);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object builder() {
		call(1837);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void cancel_all_jobs_by_name(object name) {
		pushObj(name);
		call(1838);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void task(object $instance) {
		pushObj($instance);
		call(1839);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io, thread) 

	void thread(object $instance) {
		pushObj($instance);
		call(1840);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread2(object $instance, object name, var& main) {
		pushObj($instance);
		pushObj(name);
		pushNum(main.value());
		call(1841);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread3(object $instance, object name, var& daemon, var& main) {
		pushObj($instance);
		pushObj(name);
		pushNum(daemon.value());
		pushNum(main.value());
		call(1842);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread4(object $instance, object name, var& main, var& stack_size) {
		pushObj($instance);
		pushObj(name);
		pushNum(main.value());
		pushNum(stack_size.value());
		call(1843);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread5(object $instance, object name, var& daemon, var& main, var& stack_size) {
		pushObj($instance);
		pushObj(name);
		pushNum(daemon.value());
		pushNum(main.value());
		pushNum(stack_size.value());
		call(1844);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread6(object $instance, object name, var& main, var& stack_size, object group) {
		pushObj($instance);
		pushObj(name);
		pushNum(main.value());
		pushNum(stack_size.value());
		pushObj(group);
		call(1845);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread7(object $instance, object name, var& main, object group) {
		pushObj($instance);
		pushObj(name);
		pushNum(main.value());
		pushObj(group);
		call(1846);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread8(object $instance, object name, var& daemon, var& main, var& stack_size, object group) {
		pushObj($instance);
		pushObj(name);
		pushNum(daemon.value());
		pushNum(main.value());
		pushNum(stack_size.value());
		pushObj(group);
		call(1847);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object start(object $instance, object args) {
		pushObj($instance);
		pushObj(args);
		call(1848);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object start2(object $instance) {
		pushObj($instance);
		call(1849);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object suspend(object $instance) {
		pushObj($instance);
		call(1850);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object unsuspend(object $instance) {
		pushObj($instance);
		call(1851);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object unsuspend_and_wait(object $instance) {
		pushObj($instance);
		call(1852);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object join(object $instance) {
		pushObj($instance);
		call(1853);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object interrupt(object $instance) {
		pushObj($instance);
		call(1854);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object current() {
		call(1855);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void sleep(var& mills_time) {
		pushNum(mills_time.value());
		call(1856);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void wait() {
		call(1857);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void wait2(var& mills_time) {
		pushNum(mills_time.value());
		call(1858);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void exit(var& code) {
		pushNum(code.value());
		call(1859);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(1860);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void yield() {
		call(1861);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_exception_handler(object $instance, object error_handler) {
		pushObj($instance);
		pushObj(error_handler);
		call(1862);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object set_priority(object $instance, object priority) {
		pushObj($instance);
		pushObj(priority);
		call(1863);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void remove_thread(object $instance, object t) {
		pushObj($instance);
		pushObj(t);
		call(1864);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var get_id(object $instance) {
		pushObj($instance);
		call(1865);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_native_handle(object $instance) {
		pushObj($instance);
		call(1866);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_stack_size(object $instance) {
		pushObj($instance);
		call(1867);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object get_name(object $instance) {
		pushObj($instance);
		call(1868);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var is_exited(object $instance) {
		pushObj($instance);
		call(1869);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_daemon(object $instance) {
		pushObj($instance);
		call(1870);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_started(object $instance) {
		pushObj($instance);
		call(1871);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_suspended(object $instance) {
		pushObj($instance);
		call(1872);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var set_main(object $instance, var& main) {
		pushObj($instance);
		pushNum(main.value());
		call(1873);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_active(object $instance) {
		pushObj($instance);
		call(1874);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_exit_code(object $instance) {
		pushObj($instance);
		call(1875);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object get_exception_handler(object $instance) {
		pushObj($instance);
		call(1876);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object t) {
		pushObj($instance);
		pushObj(t);
		call(1877);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std_io, thread_group) 

	void thread_group(object $instance, object name) {
		pushObj($instance);
		pushObj(name);
		call(1878);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread_group2(object $instance, object name, object priority) {
		pushObj($instance);
		pushObj(name);
		pushObj(priority);
		call(1879);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object at(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(1880);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void add(object $instance, object t) {
		pushObj($instance);
		pushObj(t);
		call(1881);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(1882);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object get_name(object $instance) {
		pushObj($instance);
		call(1883);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object get_priority(object $instance) {
		pushObj($instance);
		call(1884);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var remove(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(1885);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object get_elements(object $instance) {
		pushObj($instance);
		call(1886);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object find(object $instance, var& find_func) {
		pushObj($instance);
		pushNum(find_func.value());
		call(1887);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void kill_all(object $instance) {
		pushObj($instance);
		call(1888);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread_group3(object $instance) {
		pushObj($instance);
		call(1889);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3515);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io, thread_result) 

	object at(var& index) {
		pushNum(index.value());
		call(1891);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void thread_result(object $instance) {
		pushObj($instance);
		call(1892);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3528);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io, uncaught_exception_handler) 

	void uncaught_exception(object $instance, object err) {
		pushObj($instance);
		pushObj(err);
		call(1893);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uncaught_exception_handler(object $instance) {
		pushObj($instance);
		call(1894);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io, unix_fs) 

	var path_separator(object $instance) {
		pushObj($instance);
		call(1895);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object normalize(object $instance, object pathname) {
		pushObj($instance);
		pushObj(pathname);
		call(1896);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var get_attributes(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1897);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var check_access(object $instance, object f, var& access) {
		pushObj($instance);
		pushObj(f);
		pushNum(access.value());
		call(1898);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void unix_fs(object $instance) {
		pushObj($instance);
		call(1899);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io, win_fs) 

	var path_separator(object $instance) {
		pushObj($instance);
		call(1900);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object normalize(object $instance, object pathname) {
		pushObj($instance);
		pushObj(pathname);
		call(1901);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var get_attributes(object $instance, object f) {
		pushObj($instance);
		pushObj(f);
		call(1902);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var check_access(object $instance, object f, var& access) {
		pushObj($instance);
		pushObj(f);
		pushNum(access.value());
		call(1903);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void win_fs(object $instance) {
		pushObj($instance);
		call(1904);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_math) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(3);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var is_nan(var& v) {
		pushNum(v.value());
		call(1905);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_infinite(var& v) {
		pushNum(v.value());
		call(1906);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var sin(var& num) {
		pushNum(num.value());
		call(1907);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var cos(var& num) {
		pushNum(num.value());
		call(1908);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var tan(var& num) {
		pushNum(num.value());
		call(1909);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var sinh(var& num) {
		pushNum(num.value());
		call(1910);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var cosh(var& num) {
		pushNum(num.value());
		call(1911);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var tanh(var& num) {
		pushNum(num.value());
		call(1912);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var asin(var& num) {
		pushNum(num.value());
		call(1913);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var acos(var& num) {
		pushNum(num.value());
		call(1914);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var atan(var& num) {
		pushNum(num.value());
		call(1915);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var atan2(var& num1, var& num2) {
		pushNum(num1.value());
		pushNum(num2.value());
		call(1916);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var exp(var& num) {
		pushNum(num.value());
		call(1917);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var log(var& num) {
		pushNum(num.value());
		call(1918);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var log10(var& num) {
		pushNum(num.value());
		call(1919);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var pow(var& num, var& exp) {
		pushNum(num.value());
		pushNum(exp.value());
		call(1920);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var sqrt(var& num) {
		pushNum(num.value());
		call(1921);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var ceil(var& num) {
		pushNum(num.value());
		call(1922);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var floor(var& num) {
		pushNum(num.value());
		call(1923);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var abs(var& num) {
		pushNum(num.value());
		call(1924);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var fabs(var& num) {
		pushNum(num.value());
		call(1925);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var ldexp(var& num, var& num2) {
		pushNum(num.value());
		pushNum(num2.value());
		call(1926);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var fmod(var& num, var& num2) {
		pushNum(num.value());
		pushNum(num2.value());
		call(1927);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var round(var& num) {
		pushNum(num.value());
		call(1928);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var math_proc(var& num, var& proc) {
		pushNum(num.value());
		pushNum(proc.value());
		call(1929);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var math_proc2(var& num, var& num2, var& proc) {
		pushNum(num.value());
		pushNum(num2.value());
		pushNum(proc.value());
		call(1930);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, _nil_) 

	object to_string(object $instance) {
		pushObj($instance);
		call(1931);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void _nil_(object $instance) {
		pushObj($instance);
		call(1932);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_instance(object value) {
		pushObj(value);
		call(3399);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3516);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, _object_) 

	object to_string(object $instance) {
		pushObj($instance);
		call(1933);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void wait(object $instance) {
		pushObj($instance);
		call(1934);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void notify(object $instance) {
		pushObj($instance);
		call(1935);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void notify2(object $instance, var& mills) {
		pushObj($instance);
		pushNum(mills.value());
		call(1936);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(1937);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object guid(object $instance) {
		pushObj($instance);
		call(1938);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void _object_(object $instance) {
		pushObj($instance);
		call(1939);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(platform) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(4);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void load_library(object name) {
		pushObj(name);
		call(1943);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var free_library(object name) {
		pushObj(name);
		call(1944);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var link(object name, object library) {
		pushObj(name);
		pushObj(library);
		call(1945);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(platform, build) 

	var is_windows() {
		call(1940);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_linux() {
		call(1941);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void build(object $instance) {
		pushObj($instance);
		call(1942);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(platform_kernel) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(5);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(platform_kernel, platform) 

	void tls_init() {
		call(1946);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void static_init() {
		call(1947);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void exit(var& value) {
		pushNum(value.value());
		call(1948);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int8_array get_lib_path(object name) {
		pushObj(name);
		call(1949);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	void load_library(object name) {
		pushObj(name);
		call(1950);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var free_library(object name) {
		pushObj(name);
		call(1951);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var link_func(object name, object library) {
		pushObj(name);
		pushObj(library);
		call(1952);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var srt_init(object args) {
		pushObj(args);
		call(1953);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void platform(object $instance) {
		pushObj($instance);
		call(1954);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(platform_kernel, stack_state) 

	void stack_state(object $instance) {
		pushObj($instance);
		call(1955);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(platform_kernel, vm) 

	object get_frame_info() {
		call(1956);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object get_stack_trace(object info) {
		pushObj(info);
		call(1957);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var thread_create(var& daemon) {
		pushNum(daemon.value());
		call(1958);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object suspend(object t) {
		pushObj(t);
		call(1959);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object unsuspend(object t, var& wait) {
		pushObj(t);
		pushNum(wait.value());
		call(1960);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var get_os_time(var& tr) {
		pushNum(tr.value());
		call(1961);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object thread_join(object t) {
		pushObj(t);
		call(1962);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object kill(object t) {
		pushObj(t);
		call(1963);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object current_thread() {
		call(1964);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void exit_thread(object t, var& code) {
		pushObj(t);
		pushNum(code.value());
		call(1965);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void thread_yield() {
		call(1966);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object thread_start(object t, object args) {
		pushObj(t);
		pushObj(args);
		call(1967);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object set_thread_priority(object t, var& priority) {
		pushObj(t);
		pushNum(priority.value());
		call(1968);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void set_thread(object t) {
		pushObj(t);
		call(1969);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object thread_args() {
		call(1970);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var cores() {
		call(1971);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void vm(object $instance) {
		pushObj($instance);
		call(1972);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, ulong) 

	void ulong(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1973);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1974);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1975);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1976);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1977);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1978);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1979);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1980);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1981);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(1982);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ulong11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(1983);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_uint64 op_$not(object $instance) {
		pushObj($instance);
		call(1984);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(1985);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1986);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(1987);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1988);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1989);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1990);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1991);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1992);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1993);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1994);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1995);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1996);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1997);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1998);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1999);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2000);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2001);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2002);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2003);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2004);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2005);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2006);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2007);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2008);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2009);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2010);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2011);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2012);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2013);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2014);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2015);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2016);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2017);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2018);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2019);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2020);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2021);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2022);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2023);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2024);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2025);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2026);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2027);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2028);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2029);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2030);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2031);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2032);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2033);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2034);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2035);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2036);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2037);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2038);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2039);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2040);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2041);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2042);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2043);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2044);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2045);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2046);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2047);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2048);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2049);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2050);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2051);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2052);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2053);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2054);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2055);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2056);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2057);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2058);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2059);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2060);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2061);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2062);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2063);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2064);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2065);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void ulong12(object $instance) {
		pushObj($instance);
		call(2066);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, _long) 

	void _long(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2067);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2068);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2069);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2070);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2071);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2072);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2073);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2074);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2075);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2076);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _long11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2077);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int64 op_$not(object $instance) {
		pushObj($instance);
		call(2078);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2079);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2080);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2081);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2082);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2083);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2084);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2085);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2086);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2087);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2088);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2089);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2090);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2091);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2092);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2093);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2094);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2095);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2096);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2097);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2098);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2099);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2100);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2101);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2102);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2103);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2104);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2105);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2106);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2107);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2108);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2109);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2110);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2111);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2112);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2113);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2114);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2115);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2116);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2117);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2118);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2119);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2120);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2121);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2122);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2123);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2124);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2125);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2126);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2127);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2128);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2129);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2130);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2131);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2132);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2133);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2134);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2135);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2136);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2137);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2138);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2139);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2140);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2141);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2142);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2143);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2144);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2145);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2146);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2147);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2148);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2149);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2150);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2151);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2152);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2153);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2154);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2155);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2156);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2157);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2158);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2159);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void _long12(object $instance) {
		pushObj($instance);
		call(2160);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, uint) 

	void uint(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2161);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2162);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2163);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2164);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2165);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2166);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2167);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2168);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2169);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2170);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uint11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2171);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_uint32 op_$not(object $instance) {
		pushObj($instance);
		call(2172);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2173);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2174);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2175);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2176);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2177);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2178);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2179);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2180);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2181);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2182);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2183);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2184);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2185);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2186);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2187);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2188);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2189);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2190);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2191);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2192);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2193);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2194);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2195);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2196);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2197);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2198);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2199);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2200);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2201);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2202);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2203);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2204);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2205);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2206);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2207);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2208);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2209);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2210);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2211);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2212);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2213);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2214);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2215);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2216);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2217);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2218);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2219);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2220);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2221);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2222);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2223);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2224);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2225);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2226);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2227);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2228);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2229);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2230);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2231);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2232);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2233);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2234);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2235);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2236);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2237);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2238);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2239);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2240);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2241);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2242);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2243);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2244);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2245);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2246);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2247);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2248);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2249);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2250);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2251);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2252);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2253);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void uint12(object $instance) {
		pushObj($instance);
		call(2254);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, _int) 

	void _int(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2255);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2256);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2257);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2258);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2259);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2260);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2261);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2262);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2263);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2264);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _int11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2265);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int32 op_$not(object $instance) {
		pushObj($instance);
		call(2266);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2267);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2268);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2269);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2270);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2271);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2272);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2273);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2274);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2275);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2276);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2277);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2278);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2279);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2280);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2281);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2282);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2283);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2284);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2285);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2286);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2287);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2288);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2289);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2290);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2291);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2292);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2293);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2294);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2295);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2296);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2297);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2298);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2299);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2300);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2301);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2302);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2303);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2304);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2305);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2306);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2307);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2308);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2309);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2310);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2311);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2312);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2313);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2314);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2315);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2316);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2317);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2318);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2319);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2320);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2321);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2322);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2323);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2324);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2325);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2326);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2327);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2328);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2329);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2330);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2331);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2332);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2333);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2334);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2335);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2336);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2337);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2338);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2339);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2340);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2341);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2342);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2343);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2344);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2345);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2346);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2347);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void _int12(object $instance) {
		pushObj($instance);
		call(2348);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, ushort) 

	void ushort(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2349);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2350);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2351);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2352);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2353);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2354);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2355);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2356);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2357);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2358);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void ushort11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2359);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_uint16 op_$not(object $instance) {
		pushObj($instance);
		call(2360);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2361);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2362);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2363);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2364);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2365);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2366);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2367);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2368);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2369);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2370);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2371);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2372);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2373);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2374);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2375);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2376);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2377);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2378);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2379);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2380);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2381);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2382);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2383);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2384);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2385);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2386);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2387);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2388);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2389);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2390);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2391);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2392);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2393);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2394);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2395);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2396);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2397);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2398);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2399);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2400);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2401);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2402);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2403);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2404);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2405);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2406);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2407);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2408);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2409);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2410);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2411);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2412);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2413);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2414);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2415);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2416);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2417);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2418);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2419);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2420);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2421);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2422);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2423);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2424);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2425);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2426);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2427);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2428);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2429);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2430);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2431);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2432);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2433);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2434);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2435);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2436);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2437);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2438);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2439);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2440);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2441);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void ushort12(object $instance) {
		pushObj($instance);
		call(2442);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, _short) 

	void _short(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2443);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2444);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2445);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2446);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2447);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2448);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2449);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2450);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2451);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2452);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _short11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2453);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int16 op_$not(object $instance) {
		pushObj($instance);
		call(2454);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2455);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2456);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2457);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2458);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2459);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2460);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2461);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2462);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2463);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2464);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2465);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2466);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2467);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2468);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2469);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2470);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2471);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2472);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2473);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2474);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2475);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2476);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2477);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2478);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2479);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2480);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2481);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2482);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2483);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2484);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2485);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2486);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2487);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2488);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2489);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2490);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2491);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2492);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2493);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2494);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2495);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2496);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2497);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2498);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2499);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2500);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2501);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2502);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2503);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2504);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2505);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2506);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2507);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2508);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2509);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2510);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2511);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2512);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2513);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2514);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2515);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2516);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2517);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2518);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2519);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2520);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2521);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2522);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2523);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2524);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2525);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2526);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2527);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2528);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2529);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2530);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2531);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2532);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2533);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2534);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2535);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void _short12(object $instance) {
		pushObj($instance);
		call(2536);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, uchar) 

	void uchar(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2537);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2538);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2539);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2540);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2541);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2542);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2543);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2544);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2545);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2546);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void uchar11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2547);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_uint8 op_$not(object $instance) {
		pushObj($instance);
		call(2548);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2549);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2550);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2551);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2552);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2553);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2554);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2555);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2556);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2557);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2558);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2559);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2560);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2561);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2562);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2563);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2564);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2565);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2566);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2567);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2568);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2569);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2570);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2571);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2572);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2573);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2574);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2575);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2576);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2577);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2578);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2579);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2580);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2581);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2582);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2583);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2584);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2585);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2586);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2587);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2588);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2589);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2590);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2591);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2592);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2593);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2594);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2595);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2596);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2597);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2598);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2599);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2600);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2601);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2602);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2603);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2604);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2605);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2606);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2607);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2608);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2609);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2610);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2611);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2612);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2613);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2614);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2615);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2616);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2617);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2618);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2619);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2620);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2621);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2622);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2623);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2624);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2625);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2626);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2627);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2628);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2629);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void uchar12(object $instance) {
		pushObj($instance);
		call(2630);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, _char) 

	void _char(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2631);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2632);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2633);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2634);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2635);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2636);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2637);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2638);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2639);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2640);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _char11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2641);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int8 op_$not(object $instance) {
		pushObj($instance);
		call(2642);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2643);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2644);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2645);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2646);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2647);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2648);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2649);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2650);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2651);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2652);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2653);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2654);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2655);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2656);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2657);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2658);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2659);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2660);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2661);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2662);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2663);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2664);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2665);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2666);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2667);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2668);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2669);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2670);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2671);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2672);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2673);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2674);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2675);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2676);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2677);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2678);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2679);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2680);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2681);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2682);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2683);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2684);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2685);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2686);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2687);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2688);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2689);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2690);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2691);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2692);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2693);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2694);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2695);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2696);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2697);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2698);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2699);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2700);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2701);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2702);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2703);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2704);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2705);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2706);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2707);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2708);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2709);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2710);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2711);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2712);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2713);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2714);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2715);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2716);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2717);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2718);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2719);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2720);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2721);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2722);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2723);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_digit(var& ch) {
		pushNum(ch.value());
		call(2724);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_newline(var& ch) {
		pushNum(ch.value());
		call(2725);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_alpha(var& ch) {
		pushNum(ch.value());
		call(2726);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_alpha_num(var& ch) {
		pushNum(ch.value());
		call(2727);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var to_lower(var& ch) {
		pushNum(ch.value());
		call(2728);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var to_upper(var& ch) {
		pushNum(ch.value());
		call(2729);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void _char12(object $instance) {
		pushObj($instance);
		call(2730);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, byte) 

	void byte(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2731);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2732);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2733);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2734);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2735);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2736);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2737);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2738);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2739);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2740);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void byte11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2741);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_uint8 op_$not(object $instance) {
		pushObj($instance);
		call(2742);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2743);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2744);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2745);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2746);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2747);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2748);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2749);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2750);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2751);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2752);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2753);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2754);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2755);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2756);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2757);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2758);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2759);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2760);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2761);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2762);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2763);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2764);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2765);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2766);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2767);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2768);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2769);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2770);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2771);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2772);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2773);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2774);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2775);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2776);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2777);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2778);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2779);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2780);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2781);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2782);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2783);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2784);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2785);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2786);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2787);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2788);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2789);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2790);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2791);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2792);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2793);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2794);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2795);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2796);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2797);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2798);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2799);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2800);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2801);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2802);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2803);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2804);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2805);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2806);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2807);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2808);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2809);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2810);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2811);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2812);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2813);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2814);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2815);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2816);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2817);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2818);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2819);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2820);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2821);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2822);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2823);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_digit(var& ch) {
		pushNum(ch.value());
		call(2824);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_newline(var& ch) {
		pushNum(ch.value());
		call(2825);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_alpha(var& ch) {
		pushNum(ch.value());
		call(2826);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var is_alpha_num(var& ch) {
		pushNum(ch.value());
		call(2827);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var to_lower(var& ch) {
		pushNum(ch.value());
		call(2828);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var to_upper(var& ch) {
		pushNum(ch.value());
		call(2829);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void byte12(object $instance) {
		pushObj($instance);
		call(2830);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, _bool) 

	void _bool(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2831);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool2(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2832);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool3(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2833);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool4(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2834);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool5(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2835);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool6(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2836);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool7(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2837);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool8(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2838);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool9(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2839);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool10(object $instance, object i) {
		pushObj($instance);
		pushObj(i);
		call(2840);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void _bool11(object $instance, var& i) {
		pushObj($instance);
		pushNum(i.value());
		call(2841);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int8 op_$not(object $instance) {
		pushObj($instance);
		call(2842);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_plus(object $instance) {
		pushObj($instance);
		call(2843);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$plus_plus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2844);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_minus(object $instance) {
		pushObj($instance);
		call(2845);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$minus_minus2(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2846);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2847);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2848);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2849);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2850);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2851);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2852);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2853);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2854);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2855);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2856);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2857);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2858);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2859);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2860);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2861);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2862);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2863);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2864);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2865);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2866);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2867);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2868);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2869);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2870);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2871);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2872);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2873);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2874);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2875);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2876);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2877);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2878);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2879);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2880);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2881);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2882);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2883);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2884);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2885);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2886);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2887);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2888);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2889);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2890);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2891);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2892);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2893);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2894);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2895);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2896);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2897);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2898);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2899);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2900);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2901);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2902);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2903);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2904);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2905);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2906);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2907);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2908);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2909);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2910);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2911);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2912);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2913);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2914);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2915);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2916);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2917);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2918);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2919);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2920);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2921);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(2922);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or_equal11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2923);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var check_value(object $instance, var& new_val) {
		pushObj($instance);
		pushNum(new_val.value());
		call(2924);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void _bool12(object $instance) {
		pushObj($instance);
		call(2925);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, printable) 

	object to_string(object $instance) {
		pushObj($instance);
		call(2926);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void printable(object $instance) {
		pushObj($instance);
		call(2927);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_reflect) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(6);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_reflect, _class_) 

	void _class_(object $instance) {
		pushObj($instance);
		call(2928);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_reflect, data_entity) 

	void data_entity(object $instance) {
		pushObj($instance);
		call(2929);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_reflect, data_property) 

	void data_property(object $instance) {
		pushObj($instance);
		call(2930);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_reflect, field) 

	void field(object $instance) {
		pushObj($instance);
		call(2931);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_reflect, function) 

	void function(object $instance) {
		pushObj($instance);
		call(2932);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_reflect, reflect) 

	void reflect(object $instance, object ref_obj) {
		pushObj($instance);
		pushObj(ref_obj);
		call(2933);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void reflect2(object $instance) {
		pushObj($instance);
		call(2934);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_reflect_obj(object $instance, object value) {
		pushObj($instance);
		pushObj(value);
		call(3400);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_reflect_obj(object $instance) {
		pushObj($instance);
		call(3401);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, string) 

	void string(object $instance, _int8_array& str) {
		pushObj($instance);
		pushObj(str.handle);
		call(2935);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string2(object $instance, var& ch) {
		pushObj($instance);
		pushNum(ch.value());
		call(2936);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string3(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2937);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string4(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2938);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string5(object $instance, _int8_array& str, var& offset) {
		pushObj($instance);
		pushObj(str.handle);
		pushNum(offset.value());
		call(2939);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string6(object $instance, _int8_array& str, var& start, var& end) {
		pushObj($instance);
		pushObj(str.handle);
		pushNum(start.value());
		pushNum(end.value());
		call(2940);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object replace(object $instance, var& pos, _int8& ch) {
		pushObj($instance);
		pushNum(pos.value());
		pushNum(ch.value());
		call(2941);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object append(object $instance, var& ch) {
		pushObj($instance);
		pushNum(ch.value());
		call(2942);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var empty(object $instance) {
		pushObj($instance);
		call(2943);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object append2(object $instance, object c) {
		pushObj($instance);
		pushObj(c);
		call(2944);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object append3(object $instance, _int8_array& immstr) {
		pushObj($instance);
		pushObj(immstr.handle);
		call(2945);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object append4(object $instance, _int8_array& immstr, var& len) {
		pushObj($instance);
		pushObj(immstr.handle);
		pushNum(len.value());
		call(2946);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus(object $instance, _int8_array& immstr) {
		pushObj($instance);
		pushObj(immstr.handle);
		call(2947);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_lower(object $instance) {
		pushObj($instance);
		call(2948);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus2(object $instance, object obj) {
		pushObj($instance);
		pushObj(obj);
		call(2949);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus3(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2950);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(2951);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, _int8_array& str) {
		pushObj($instance);
		pushObj(str.handle);
		call(2952);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2953);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, _int8_array& str) {
		pushObj($instance);
		pushObj(str.handle);
		call(2954);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2955);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void op_$equals(object $instance, _int8_array& immstr) {
		pushObj($instance);
		pushObj(immstr.handle);
		call(2956);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int8 at(object $instance, var& pos) {
		pushObj($instance);
		pushNum(pos.value());
		call(2957);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$array_at(object $instance, var& pos) {
		pushObj($instance);
		pushNum(pos.value());
		call(2958);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void fmt(var& _fmt) {
		pushNum(_fmt.value());
		call(2959);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_precision(var& prec) {
		pushNum(prec.value());
		call(2960);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int8_array get_data(object $instance) {
		pushObj($instance);
		call(2961);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int8_array get_elements(object $instance) {
		pushObj($instance);
		call(2962);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	_int8_array to_array(object $instance) {
		pushObj($instance);
		call(2963);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	var starts_with(object $instance, object prefix) {
		pushObj($instance);
		pushObj(prefix);
		call(2964);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var find(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2965);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var ends_with(object $instance, object postfix) {
		pushObj($instance);
		pushObj(postfix);
		call(2966);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object op_$plus4(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2967);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus5(object $instance, _int8& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2968);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus6(object $instance, _int16& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2969);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus7(object $instance, _int32& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2970);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus8(object $instance, _int64& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2971);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus9(object $instance, _uint8& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2972);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus10(object $instance, _uint16& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2973);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus11(object $instance, _uint32& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2974);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus12(object $instance, _uint64& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2975);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var size(object $instance) {
		pushObj($instance);
		call(2976);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object parse(var& number) {
		pushNum(number.value());
		call(2977);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2978);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal2(object $instance, _int8& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2979);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal3(object $instance, _uint8& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2980);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal4(object $instance, _int16& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2981);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal5(object $instance, _uint16& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2982);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal6(object $instance, _int32& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2983);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal7(object $instance, _uint32& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2984);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal8(object $instance, _int64& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2985);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal9(object $instance, _uint64& num) {
		pushObj($instance);
		pushNum(num.value());
		call(2986);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal10(object $instance, object obj) {
		pushObj($instance);
		pushObj(obj);
		call(2987);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal11(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2988);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal12(object $instance, _int8_array& str) {
		pushObj($instance);
		pushObj(str.handle);
		call(2989);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object substring(object $instance, var& start_pos, var& end_pos) {
		pushObj($instance);
		pushNum(start_pos.value());
		pushNum(end_pos.value());
		call(2990);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object bounds_error(object $instance, var& pos) {
		pushObj($instance);
		pushNum(pos.value());
		call(2991);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object hash(object $instance) {
		pushObj($instance);
		call(2992);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void string7(object $instance) {
		pushObj($instance);
		call(2993);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3518);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, string_builder) 

	void set_resize_capacity(object $instance, var& new_capacity) {
		pushObj($instance);
		pushNum(new_capacity.value());
		call(2994);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string_builder(object $instance, _int8_array& str) {
		pushObj($instance);
		pushObj(str.handle);
		call(2995);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string_builder2(object $instance, var& ch) {
		pushObj($instance);
		pushNum(ch.value());
		call(2996);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string_builder3(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2997);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string_builder4(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2998);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string_builder5(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(2999);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void string_builder6(object $instance, _int8_array& str, var& offset) {
		pushObj($instance);
		pushObj(str.handle);
		pushNum(offset.value());
		call(3000);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var space_available(object $instance, var& space_required) {
		pushObj($instance);
		pushNum(space_required.value());
		call(3001);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void expand(object $instance, var& space_required) {
		pushObj($instance);
		pushNum(space_required.value());
		call(3002);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object replace(object $instance, var& pos, _int8& ch) {
		pushObj($instance);
		pushNum(pos.value());
		pushNum(ch.value());
		call(3003);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object append(object $instance, var& ch) {
		pushObj($instance);
		pushNum(ch.value());
		call(3004);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int8_array get_elements(object $instance) {
		pushObj($instance);
		call(3005);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	object append2(object $instance, object c) {
		pushObj($instance);
		pushObj(c);
		call(3006);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object append3(object $instance, _int8_array& immstr) {
		pushObj($instance);
		pushObj(immstr.handle);
		call(3007);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object append4(object $instance, _int8_array& immstr, var& len) {
		pushObj($instance);
		pushObj(immstr.handle);
		pushNum(len.value());
		call(3008);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus(object $instance, _int8_array& immstr) {
		pushObj($instance);
		pushObj(immstr.handle);
		call(3009);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus2(object $instance, object obj) {
		pushObj($instance);
		pushObj(obj);
		call(3010);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3011);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, _int8_array& str) {
		pushObj($instance);
		pushObj(str.handle);
		call(3012);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(3013);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, _int8_array& str) {
		pushObj($instance);
		pushObj(str.handle);
		call(3014);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(3015);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void op_$equals(object $instance, _int8_array& immstr) {
		pushObj($instance);
		pushObj(immstr.handle);
		call(3016);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int8 at(object $instance, var& pos) {
		pushObj($instance);
		pushNum(pos.value());
		call(3017);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$array_at(object $instance, var& pos) {
		pushObj($instance);
		pushNum(pos.value());
		call(3018);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void fmt(var& _fmt) {
		pushNum(_fmt.value());
		call(3019);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_precision(var& prec) {
		pushNum(prec.value());
		call(3020);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	_int8_array to_array(object $instance) {
		pushObj($instance);
		call(3021);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	var starts_with(object $instance, object prefix) {
		pushObj($instance);
		pushObj(prefix);
		call(3022);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var find(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(3023);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var find2(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(3024);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var ends_with(object $instance, object postfix) {
		pushObj($instance);
		pushObj(postfix);
		call(3025);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object op_$plus3(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(3026);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var size(object $instance) {
		pushObj($instance);
		call(3027);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object parse(object $instance, var& number) {
		pushObj($instance);
		pushNum(number.value());
		call(3028);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(3029);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal2(object $instance, object obj) {
		pushObj($instance);
		pushObj(obj);
		call(3030);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal3(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(3031);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int8_array get_data(object $instance) {
		pushObj($instance);
		call(3032);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	object op_$plus_equal4(object $instance, object str) {
		pushObj($instance);
		pushObj(str);
		call(3033);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$plus_equal5(object $instance, _int8_array& str) {
		pushObj($instance);
		pushObj(str.handle);
		call(3034);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object substring(object $instance, var& startPos, var& endPos) {
		pushObj($instance);
		pushNum(startPos.value());
		pushNum(endPos.value());
		call(3035);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object bounds_error(object $instance, var& pos) {
		pushObj($instance);
		pushNum(pos.value());
		call(3036);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object hash(object $instance) {
		pushObj($instance);
		call(3037);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void string_builder7(object $instance) {
		pushObj($instance);
		call(3038);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3519);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, time) 

	var nano_time() {
		call(3039);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_os_time(object tr) {
		pushObj(tr);
		call(3040);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var nano_to_micro(var& nano) {
		pushNum(nano.value());
		call(3041);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var nano_to_mills(var& nano) {
		pushNum(nano.value());
		call(3042);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var nano_to_secs(var& nano) {
		pushNum(nano.value());
		call(3043);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var mills_time() {
		call(3044);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void sleep(var& time) {
		pushNum(time.value());
		call(3045);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void sleep2(object tm_unit, var& time) {
		pushObj(tm_unit);
		pushNum(time.value());
		call(3046);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void wait(object tm_unit, var& time) {
		pushObj(tm_unit);
		pushNum(time.value());
		call(3047);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var adjust_time(var& usec) {
		pushNum(usec.value());
		call(3048);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var measure(object tm_unit, var& block) {
		pushObj(tm_unit);
		pushNum(block.value());
		call(3049);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void time(object $instance) {
		pushObj($instance);
		call(3050);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3520);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, unique) 

	object hash(object $instance) {
		pushObj($instance);
		call(3051);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object guid(object $instance) {
		pushObj($instance);
		call(3052);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void unique(object $instance) {
		pushObj($instance);
		call(3053);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(7);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common, constants) 

	void constants(object $instance) {
		pushObj($instance);
		call(3055);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3521);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common, constants_strings) 

	void strings(object $instance) {
		pushObj($instance);
		call(3054);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3522);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_gpio) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(8);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_core) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(9);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var anon_func$3523(object t1, object t2) {
		pushObj(t1);
		pushObj(t2);
		call(3523);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(common_network_core, request) 

	var_array get_network_quality() {
		call(3060);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	var get_signal_strength() {
		call(3061);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var write(object rdata) {
		pushObj(rdata);
		call(3062);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void process_result(object raw, object rdata) {
		pushObj(raw);
		pushObj(rdata);
		call(3063);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var read(object rdata) {
		pushObj(rdata);
		call(3064);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void listen(object rdata) {
		pushObj(rdata);
		call(3065);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var rw_inf(object rdata, var& rw, var& count) {
		pushObj(rdata);
		pushNum(rw.value());
		pushNum(count.value());
		call(3066);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void request(object $instance) {
		pushObj($instance);
		call(3067);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_core, request_data) 

	object add_item(object $instance, object key, object value) {
		pushObj($instance);
		pushObj(key);
		pushObj(value);
		call(3068);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3069);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object at(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3070);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3071);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void request_data(object $instance) {
		pushObj($instance);
		call(3074);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_request_string(object $instance) {
		pushObj($instance);
		call(3431);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(common_network_core, request_data_request_item) 

	void request_item(object $instance, object k, object v) {
		pushObj($instance);
		pushObj(k);
		pushObj(v);
		call(3072);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void request_item2(object $instance) {
		pushObj($instance);
		call(3073);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_request) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(10);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_request, acknowledge_request) 

	void send(object $instance) {
		pushObj($instance);
		call(3075);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void acknowledge_request(object $instance) {
		pushObj($instance);
		call(3076);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_request, command_request) 

	object listen(object $instance) {
		pushObj($instance);
		call(3077);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var send_command(object $instance, object req, var& ack, var& timeout) {
		pushObj($instance);
		pushObj(req);
		pushNum(ack.value());
		pushNum(timeout.value());
		call(3078);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void command_request(object $instance) {
		pushObj($instance);
		call(3079);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_request, flight_data_request) 

	object _get(object $instance) {
		pushObj($instance);
		call(3080);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void post(object $instance, object data) {
		pushObj($instance);
		pushObj(data);
		call(3081);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void flight_data_request(object $instance) {
		pushObj($instance);
		call(3082);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_request, flight_mode_change_request) 

	var change_mode(object $instance, object mode) {
		pushObj($instance);
		pushObj(mode);
		call(3083);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void flight_mode_change_request(object $instance) {
		pushObj($instance);
		call(3084);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_request, handshake_request) 

	void handshake(object $instance) {
		pushObj($instance);
		call(3085);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void handshake_request(object $instance) {
		pushObj($instance);
		call(3086);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_request, network_scan_request) 

	var_array scan_network(object $instance) {
		pushObj($instance);
		call(3087);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	void network_scan_request(object $instance) {
		pushObj($instance);
		call(3088);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_request, signal_strength_request) 

	var get_strength(object $instance) {
		pushObj($instance);
		call(3089);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void signal_strength_request(object $instance) {
		pushObj($instance);
		call(3090);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_response) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(11);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_response, command_response) 

	void command_response(object $instance, object cmd, object req) {
		pushObj($instance);
		pushObj(cmd);
		pushObj(req);
		call(3091);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void command_response2(object $instance) {
		pushObj($instance);
		call(3092);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_data_response, flight_data_response) 

	void flight_data_response(object $instance, var& bat_lvl, var& altitude, var& speed) {
		pushObj($instance);
		pushNum(bat_lvl.value());
		pushNum(altitude.value());
		pushNum(speed.value());
		call(3093);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void flight_data_response2(object $instance) {
		pushObj($instance);
		call(3094);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_driver) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(12);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_driver, nrf24) 

	void nrf24(object $instance) {
		pushObj($instance);
		call(3107);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_rf_lvl(object $instance, var& level) {
		pushObj($instance);
		pushNum(level.value());
		call(3108);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_data_rate(object $instance, var& level) {
		pushObj($instance);
		pushNum(level.value());
		call(3109);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void dump(object $instance) {
		pushObj($instance);
		call(3110);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void update_retry_count(object $instance, var& delay, var& count) {
		pushObj($instance);
		pushNum(delay.value());
		pushNum(count.value());
		call(3111);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void shut_down(object $instance) {
		pushObj($instance);
		call(3112);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var_array get_network_quality(object $instance) {
		pushObj($instance);
		call(3113);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	var get_signal_strength(object $instance) {
		pushObj($instance);
		call(3114);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var read(object $instance, object response) {
		pushObj($instance);
		pushObj(response);
		call(3115);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void listen(object $instance, object response) {
		pushObj($instance);
		pushObj(response);
		call(3116);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var send(object $instance, object response) {
		pushObj($instance);
		pushObj(response);
		call(3117);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void check_state(object $instance) {
		pushObj($instance);
		call(3118);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(13);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network, network) 

	void handshake() {
		call(3119);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object listen() {
		call(3120);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var change_mode(object mode) {
		pushObj(mode);
		call(3121);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var get_signal_strength() {
		call(3122);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var_array scan_network() {
		call(3123);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	object get_flight_data() {
		call(3124);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void post_flight_data(object data) {
		pushObj(data);
		call(3125);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void send_acknowledge() {
		call(3126);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void network(object $instance) {
		pushObj($instance);
		call(3127);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_INSTANCE() {
		call(3432);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(common_network_remote) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(14);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void initialize(object net) {
		pushObj(net);
		call(3142);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_remote, acknowledge_request_impl) 

	void acknowledge_request_impl(object $instance) {
		pushObj($instance);
		call(3128);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void send(object $instance) {
		pushObj($instance);
		call(3129);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_remote, command_request_impl) 

	object listen(object $instance) {
		pushObj($instance);
		call(3130);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var send_command(object $instance, object req, var& ack, var& timeout) {
		pushObj($instance);
		pushObj(req);
		pushNum(ack.value());
		pushNum(timeout.value());
		call(3131);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void command_request_impl(object $instance) {
		pushObj($instance);
		call(3132);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_remote, flight_data_request_impl) 

	object _get(object $instance) {
		pushObj($instance);
		call(3133);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void post(object $instance, object data) {
		pushObj($instance);
		pushObj(data);
		call(3134);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void flight_data_request_impl(object $instance) {
		pushObj($instance);
		call(3135);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_remote, flight_mode_change_request_impl) 

	void flight_mode_change_request_impl(object $instance) {
		pushObj($instance);
		call(3136);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var change_mode(object $instance, object mode) {
		pushObj($instance);
		pushObj(mode);
		call(3137);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(common_network_remote, handshake_request_impl) 

	void handshake_request_impl(object $instance) {
		pushObj($instance);
		call(3138);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void handshake(object $instance) {
		pushObj($instance);
		call(3139);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_remote, network_scan_request_impl) 

	var_array scan_network(object $instance) {
		pushObj($instance);
		call(3140);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	void network_scan_request_impl(object $instance) {
		pushObj($instance);
		call(3141);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(common_network_remote, signal_strength_request_impl) 

	var get_strength(object $instance) {
		pushObj($instance);
		call(3143);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void signal_strength_request_impl(object $instance) {
		pushObj($instance);
		call(3144);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(main) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(15);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void setup_conn_tracker() {
		call(3145);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void main(object args) {
		pushObj(args);
		call(3146);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3524);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void anon_func$3535() {
		call(3535);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void anon_func$3536() {
		call(3536);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void anon_func$3537() {
		call(3537);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void anon_func$3538() {
		call(3538);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_driver) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(16);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_driver, ssd1306) 

	void ssd1306(object $instance) {
		pushObj($instance);
		call(3154);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3155);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void dim_display(object $instance, var& yes) {
		pushObj($instance);
		pushNum(yes.value());
		call(3156);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void update_display(object $instance) {
		pushObj($instance);
		call(3157);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void draw_img(object $instance, var& x, var& y, var& width, var& height, var& img_width, var& img_height, var_array& bytes) {
		pushObj($instance);
		pushNum(x.value());
		pushNum(y.value());
		pushNum(width.value());
		pushNum(height.value());
		pushNum(img_width.value());
		pushNum(img_height.value());
		pushObj(bytes.handle);
		call(3158);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void draw_word(object $instance, var& x, var& y, var& text_size, var& width, var& transx, _int8_array& text) {
		pushObj($instance);
		pushNum(x.value());
		pushNum(y.value());
		pushNum(text_size.value());
		pushNum(width.value());
		pushNum(transx.value());
		pushObj(text.handle);
		call(3159);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void shutdown(object $instance) {
		pushObj($instance);
		call(3160);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_layout_config) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(17);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void configure_layout() {
		call(3161);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void start() {
		call(3162);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_layout) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(18);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3525);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void anon_func$3539(object args) {
		pushObj(args);
		call(3539);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void anon_func$3540(object args) {
		pushObj(args);
		call(3540);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void anon_func$3541(object args) {
		pushObj(args);
		call(3541);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_layout, home_screen) 

	void home_screen(object $instance, object container, var& id) {
		pushObj($instance);
		pushObj(container);
		pushNum(id.value());
		call(3163);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void configure(object $instance) {
		pushObj($instance);
		call(3164);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void on_create(object $instance) {
		pushObj($instance);
		call(3165);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void on_destroy(object $instance) {
		pushObj($instance);
		call(3166);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void home_screen2(object $instance) {
		pushObj($instance);
		call(3167);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3526);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_res) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(19);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_res_drawable) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(20);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void $03internal_static_init() {
		call(3527);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_res, resources) 

	void setup() {
		call(3168);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_drawable(var& res_id) {
		pushNum(res_id.value());
		call(3169);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void resources(object $instance) {
		pushObj($instance);
		call(3170);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_res() {
		call(3433);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(ui_support) 

	void __srt_global(object $instance) {
		pushObj($instance);
		call(21);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants) 

	void constants(object $instance) {
		pushObj($instance);
		call(3180);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_view) 

	void view(object $instance) {
		pushObj($instance);
		call(3171);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_window) 

	void window(object $instance) {
		pushObj($instance);
		call(3172);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_lifecycle) 

	void lifecycle(object $instance) {
		pushObj($instance);
		call(3173);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_time) 

	void time(object $instance) {
		pushObj($instance);
		call(3174);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_layout) 

	void layout(object $instance) {
		pushObj($instance);
		call(3175);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_margins) 

	void margins(object $instance) {
		pushObj($instance);
		call(3176);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_resource) 

	void resource(object $instance) {
		pushObj($instance);
		call(3178);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_resource_id) 

	void id(object $instance) {
		pushObj($instance);
		call(3177);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, constants_standard) 

	void standard(object $instance) {
		pushObj($instance);
		call(3179);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, drawable) 

	void drawable(object $instance, var& id, var_array& bytes, var& w, var& h) {
		pushObj($instance);
		pushNum(id.value());
		pushObj(bytes.handle);
		pushNum(w.value());
		pushNum(h.value());
		call(3181);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void drawable2(object $instance) {
		pushObj($instance);
		call(3182);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, fragment) 

	void fragment(object $instance, object container, var& id) {
		pushObj($instance);
		pushObj(container);
		pushNum(id.value());
		call(3183);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_view(object $instance, object v) {
		pushObj($instance);
		pushObj(v);
		call(3184);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object find_view_by_id(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3185);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void on_create(object $instance) {
		pushObj($instance);
		call(3186);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void invalidate_refrences(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3187);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void on_destroy(object $instance) {
		pushObj($instance);
		call(3188);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void navigate(object $instance, var& screen, var& finish) {
		pushObj($instance);
		pushNum(screen.value());
		pushNum(finish.value());
		call(3189);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void draw(object $instance) {
		pushObj($instance);
		call(3190);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void configure(object $instance) {
		pushObj($instance);
		call(3191);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void fragment2(object $instance) {
		pushObj($instance);
		call(3192);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, image_view) 

	void image_view(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3193);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var transform_height(object $instance) {
		pushObj($instance);
		call(3194);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var transform_width(object $instance) {
		pushObj($instance);
		call(3195);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object set_drawable(object $instance, var& res_id) {
		pushObj($instance);
		pushNum(res_id.value());
		call(3196);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void draw(object $instance) {
		pushObj($instance);
		call(3197);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void on_destroy(object $instance) {
		pushObj($instance);
		call(3198);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void image_view2(object $instance) {
		pushObj($instance);
		call(3199);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, lifecycle) 

	void on_create(object $instance) {
		pushObj($instance);
		call(3200);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void on_destroy(object $instance) {
		pushObj($instance);
		call(3201);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void lifecycle(object $instance) {
		pushObj($instance);
		call(3202);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, margin) 

	void margin(object $instance) {
		pushObj($instance);
		call(3203);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, text_view) 

	void text_view(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3204);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var transform_height(object $instance) {
		pushObj($instance);
		call(3205);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var transform_width(object $instance) {
		pushObj($instance);
		call(3206);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void draw(object $instance) {
		pushObj($instance);
		call(3207);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void on_destroy(object $instance) {
		pushObj($instance);
		call(3208);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void text_view2(object $instance) {
		pushObj($instance);
		call(3209);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_text(object $instance, object value) {
		pushObj($instance);
		pushObj(value);
		call(3471);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_text_size(object $instance, var& value) {
		pushObj($instance);
		pushNum(value.value());
		call(3472);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_support, view) 

	void view(object $instance) {
		pushObj($instance);
		call(3210);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void view2(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3211);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void on_destroy(object $instance) {
		pushObj($instance);
		call(3212);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object set_width(object $instance, var& size) {
		pushObj($instance);
		pushNum(size.value());
		call(3213);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var transform_height(object $instance) {
		pushObj($instance);
		call(3214);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var transform_width(object $instance) {
		pushObj($instance);
		call(3215);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var has_ref(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3216);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void invalidate(object $instance) {
		pushObj($instance);
		call(3217);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void draw(object $instance) {
		pushObj($instance);
		call(3218);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object set_height(object $instance, var& size) {
		pushObj($instance);
		pushNum(size.value());
		call(3219);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object set_visibility(object $instance, var& visible) {
		pushObj($instance);
		pushNum(visible.value());
		call(3220);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_left_of(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3221);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_start_of(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3222);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_right_of(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3223);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_top_of(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3224);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bottom_of(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3225);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object set_gravity(object $instance, var& grav) {
		pushObj($instance);
		pushNum(grav.value());
		call(3226);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object above(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3227);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object below(object $instance, var& id) {
		pushObj($instance);
		pushNum(id.value());
		call(3228);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object set_margin_left(object $instance, var& size) {
		pushObj($instance);
		pushNum(size.value());
		call(3229);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object set_margin_right(object $instance, var& size) {
		pushObj($instance);
		pushNum(size.value());
		call(3230);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object set_margin_top(object $instance, var& size) {
		pushObj($instance);
		pushNum(size.value());
		call(3231);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object set_margin_bottom(object $instance, var& size) {
		pushObj($instance);
		pushNum(size.value());
		call(3232);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(ui_support, window) 

	void window(object $instance, _int8_array& name) {
		pushObj($instance);
		pushObj(name.handle);
		call(3233);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void start_ui() {
		call(3234);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void navigate(object $instance, var& screen) {
		pushObj($instance);
		pushNum(screen.value());
		call(3235);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void starter_fragment(object $instance, var& frag) {
		pushObj($instance);
		pushNum(frag.value());
		call(3236);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void draw_text(object $instance, object tv) {
		pushObj($instance);
		pushObj(tv);
		call(3237);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void draw_img(object $instance, object iv) {
		pushObj($instance);
		pushObj(iv);
		call(3238);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void show(object $instance) {
		pushObj($instance);
		call(3239);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_fragment(object $instance, object frag) {
		pushObj($instance);
		pushObj(frag);
		call(3240);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void start_updater(object $instance) {
		pushObj($instance);
		call(3241);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void window2(object $instance) {
		pushObj($instance);
		call(3242);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, loopable$std_io_thread$) 

	object get_elements(object $instance) {
		pushObj($instance);
		call(22);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void loopable(object $instance) {
		pushObj($instance);
		call(23);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, integer$_uint64$) 

	void integer(object $instance, var& initial_val) {
		pushObj($instance);
		pushNum(initial_val.value());
		call(24);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(25);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(26);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(27);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(28);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(29);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(30);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(31);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(32);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(33);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(34);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(35);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(36);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(37);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(38);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(39);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(40);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(41);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(42);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(43);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(44);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(45);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(46);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(47);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(48);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(49);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(50);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(51);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(52);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(53);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(54);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(55);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(56);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(57);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(58);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(59);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(60);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(61);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(62);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(63);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(64);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(65);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(66);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(67);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(68);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(69);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(70);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(71);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(72);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(73);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(74);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(75);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(76);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(77);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(78);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(79);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(80);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(81);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(82);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(83);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(84);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(85);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(86);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(87);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(88);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(89);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(90);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(91);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(92);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(93);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(94);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(95);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(96);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(97);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(98);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(99);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(100);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(101);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(102);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(103);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(104);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(105);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(106);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(107);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(108);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(109);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(110);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(111);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(112);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(113);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(114);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(115);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(116);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(117);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(118);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(119);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(120);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(121);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(122);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(123);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(124);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(125);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(126);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(127);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(128);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(129);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(130);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(131);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(132);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(133);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(134);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(135);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(136);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(137);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(138);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(139);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(140);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(141);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(142);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(143);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(144);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(145);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(146);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(147);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(148);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(149);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(150);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(151);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(152);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(153);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(154);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(155);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(156);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(157);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(158);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(159);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(160);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(161);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(162);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(163);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(164);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(165);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(166);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(167);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(168);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(169);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(170);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(171);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(172);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(173);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(174);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(175);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(176);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(177);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(178);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(179);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(180);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(181);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(182);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(183);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(184);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(185);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(186);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(187);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(188);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(189);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(190);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(191);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(192);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(193);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(194);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(195);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(196);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(197);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(198);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(199);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(200);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(201);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint64 op_$equals(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(202);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(203);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_uint64 parse(object s) {
		pushObj(s);
		call(204);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_ulong(object $instance) {
		pushObj($instance);
		call(205);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_long(object $instance) {
		pushObj($instance);
		call(206);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uint(object $instance) {
		pushObj($instance);
		call(207);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_int(object $instance) {
		pushObj($instance);
		call(208);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_ushort(object $instance) {
		pushObj($instance);
		call(209);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_short(object $instance) {
		pushObj($instance);
		call(210);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uchar(object $instance) {
		pushObj($instance);
		call(211);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_char(object $instance) {
		pushObj($instance);
		call(212);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_byte(object $instance) {
		pushObj($instance);
		call(213);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bool(object $instance) {
		pushObj($instance);
		call(214);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_uint64 get_value(object $instance) {
		pushObj($instance);
		call(215);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void integer2(object $instance) {
		pushObj($instance);
		call(216);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, integer$_int64$) 

	void integer(object $instance, var& initial_val) {
		pushObj($instance);
		pushNum(initial_val.value());
		call(217);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(218);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(219);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(220);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(221);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(222);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(223);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(224);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(225);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(226);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(227);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(228);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(229);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(230);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(231);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(232);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(233);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(234);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(235);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(236);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(237);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(238);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(239);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(240);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(241);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(242);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(243);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(244);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(245);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(246);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(247);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(248);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(249);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(250);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(251);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(252);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(253);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(254);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(255);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(256);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(257);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(258);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(259);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(260);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(261);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(262);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(263);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(264);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(265);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(266);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(267);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(268);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(269);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(270);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(271);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(272);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(273);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(274);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(275);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(276);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(277);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(278);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(279);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(280);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(281);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(282);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(283);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(284);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(285);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(286);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(287);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(288);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(289);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(290);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(291);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(292);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(293);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(294);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(295);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(296);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(297);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(298);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(299);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(300);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(301);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(302);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(303);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(304);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(305);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(306);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(307);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(308);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(309);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(310);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(311);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(312);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(313);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(314);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(315);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(316);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(317);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(318);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(319);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(320);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(321);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(322);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(323);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(324);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(325);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(326);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(327);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(328);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(329);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(330);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(331);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(332);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(333);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(334);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(335);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(336);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(337);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(338);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(339);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(340);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(341);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(342);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(343);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(344);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(345);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(346);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(347);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(348);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(349);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(350);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(351);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(352);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(353);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(354);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(355);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(356);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(357);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(358);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(359);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(360);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(361);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(362);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(363);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(364);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(365);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(366);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(367);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(368);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(369);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(370);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(371);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(372);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(373);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(374);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(375);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(376);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(377);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(378);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(379);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(380);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(381);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(382);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(383);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(384);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(385);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(386);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(387);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(388);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(389);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(390);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(391);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(392);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(393);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(394);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int64 op_$equals(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(395);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(396);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int64 parse(object s) {
		pushObj(s);
		call(397);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_ulong(object $instance) {
		pushObj($instance);
		call(398);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_long(object $instance) {
		pushObj($instance);
		call(399);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uint(object $instance) {
		pushObj($instance);
		call(400);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_int(object $instance) {
		pushObj($instance);
		call(401);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_ushort(object $instance) {
		pushObj($instance);
		call(402);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_short(object $instance) {
		pushObj($instance);
		call(403);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uchar(object $instance) {
		pushObj($instance);
		call(404);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_char(object $instance) {
		pushObj($instance);
		call(405);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_byte(object $instance) {
		pushObj($instance);
		call(406);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bool(object $instance) {
		pushObj($instance);
		call(407);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int64 get_value(object $instance) {
		pushObj($instance);
		call(408);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int64 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void integer2(object $instance) {
		pushObj($instance);
		call(409);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, integer$_uint32$) 

	void integer(object $instance, var& initial_val) {
		pushObj($instance);
		pushNum(initial_val.value());
		call(410);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(411);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(412);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(413);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(414);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(415);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(416);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(417);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(418);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(419);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(420);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(421);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(422);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(423);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(424);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(425);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(426);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(427);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(428);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(429);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(430);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(431);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(432);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(433);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(434);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(435);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(436);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(437);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(438);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(439);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(440);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(441);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(442);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(443);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(444);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(445);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(446);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(447);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(448);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(449);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(450);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(451);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(452);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(453);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(454);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(455);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(456);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(457);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(458);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(459);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(460);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(461);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(462);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(463);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(464);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(465);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(466);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(467);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(468);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(469);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(470);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(471);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(472);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(473);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(474);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(475);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(476);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(477);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(478);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(479);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(480);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(481);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(482);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(483);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(484);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(485);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(486);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(487);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(488);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(489);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(490);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(491);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(492);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(493);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(494);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(495);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(496);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(497);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(498);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(499);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(500);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(501);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(502);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(503);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(504);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(505);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(506);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(507);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(508);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(509);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(510);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(511);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(512);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(513);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(514);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(515);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(516);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(517);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(518);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(519);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(520);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(521);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(522);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(523);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(524);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(525);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(526);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(527);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(528);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(529);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(530);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(531);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(532);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(533);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(534);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(535);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(536);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(537);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(538);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(539);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(540);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(541);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(542);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(543);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(544);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(545);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(546);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(547);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(548);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(549);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(550);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(551);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(552);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(553);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(554);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(555);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(556);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(557);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(558);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(559);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(560);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(561);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(562);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(563);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(564);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(565);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(566);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(567);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(568);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(569);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(570);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(571);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(572);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(573);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(574);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(575);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(576);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(577);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(578);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(579);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(580);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(581);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(582);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(583);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(584);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(585);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(586);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(587);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint32 op_$equals(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(588);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(589);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_uint32 parse(object s) {
		pushObj(s);
		call(590);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_ulong(object $instance) {
		pushObj($instance);
		call(591);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_long(object $instance) {
		pushObj($instance);
		call(592);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uint(object $instance) {
		pushObj($instance);
		call(593);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_int(object $instance) {
		pushObj($instance);
		call(594);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_ushort(object $instance) {
		pushObj($instance);
		call(595);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_short(object $instance) {
		pushObj($instance);
		call(596);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uchar(object $instance) {
		pushObj($instance);
		call(597);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_char(object $instance) {
		pushObj($instance);
		call(598);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_byte(object $instance) {
		pushObj($instance);
		call(599);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bool(object $instance) {
		pushObj($instance);
		call(600);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_uint32 get_value(object $instance) {
		pushObj($instance);
		call(601);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void integer2(object $instance) {
		pushObj($instance);
		call(602);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, integer$_int32$) 

	void integer(object $instance, var& initial_val) {
		pushObj($instance);
		pushNum(initial_val.value());
		call(603);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(604);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(605);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(606);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(607);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(608);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(609);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(610);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(611);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(612);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(613);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(614);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(615);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(616);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(617);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(618);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(619);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(620);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(621);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(622);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(623);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(624);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(625);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(626);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(627);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(628);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(629);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(630);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(631);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(632);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(633);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(634);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(635);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(636);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(637);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(638);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(639);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(640);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(641);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(642);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(643);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(644);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(645);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(646);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(647);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(648);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(649);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(650);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(651);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(652);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(653);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(654);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(655);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(656);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(657);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(658);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(659);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(660);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(661);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(662);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(663);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(664);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(665);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(666);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(667);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(668);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(669);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(670);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(671);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(672);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(673);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(674);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(675);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(676);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(677);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(678);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(679);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(680);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(681);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(682);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(683);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(684);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(685);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(686);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(687);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(688);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(689);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(690);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(691);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(692);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(693);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(694);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(695);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(696);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(697);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(698);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(699);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(700);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(701);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(702);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(703);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(704);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(705);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(706);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(707);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(708);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(709);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(710);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(711);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(712);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(713);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(714);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(715);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(716);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(717);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(718);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(719);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(720);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(721);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(722);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(723);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(724);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(725);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(726);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(727);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(728);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(729);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(730);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(731);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(732);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(733);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(734);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(735);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(736);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(737);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(738);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(739);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(740);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(741);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(742);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(743);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(744);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(745);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(746);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(747);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(748);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(749);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(750);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(751);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(752);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(753);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(754);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(755);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(756);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(757);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(758);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(759);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(760);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(761);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(762);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(763);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(764);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(765);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(766);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(767);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(768);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(769);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(770);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(771);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(772);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(773);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(774);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(775);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(776);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(777);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(778);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(779);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(780);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int32 op_$equals(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(781);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(782);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int32 parse(object s) {
		pushObj(s);
		call(783);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_ulong(object $instance) {
		pushObj($instance);
		call(784);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_long(object $instance) {
		pushObj($instance);
		call(785);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uint(object $instance) {
		pushObj($instance);
		call(786);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_int(object $instance) {
		pushObj($instance);
		call(787);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_ushort(object $instance) {
		pushObj($instance);
		call(788);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_short(object $instance) {
		pushObj($instance);
		call(789);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uchar(object $instance) {
		pushObj($instance);
		call(790);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_char(object $instance) {
		pushObj($instance);
		call(791);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_byte(object $instance) {
		pushObj($instance);
		call(792);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bool(object $instance) {
		pushObj($instance);
		call(793);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int32 get_value(object $instance) {
		pushObj($instance);
		call(794);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int32 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void integer2(object $instance) {
		pushObj($instance);
		call(795);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, integer$_uint16$) 

	void integer(object $instance, var& initial_val) {
		pushObj($instance);
		pushNum(initial_val.value());
		call(796);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(797);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(798);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(799);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(800);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(801);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(802);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(803);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(804);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(805);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(806);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(807);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(808);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(809);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(810);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(811);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(812);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(813);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(814);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(815);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(816);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(817);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(818);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(819);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(820);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(821);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(822);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(823);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(824);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(825);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(826);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(827);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(828);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(829);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(830);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(831);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(832);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(833);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(834);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(835);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(836);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(837);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(838);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(839);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(840);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(841);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(842);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(843);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(844);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(845);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(846);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(847);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(848);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(849);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(850);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(851);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(852);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(853);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(854);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(855);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(856);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(857);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(858);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(859);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(860);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(861);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(862);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(863);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(864);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(865);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(866);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(867);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(868);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(869);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(870);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(871);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(872);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(873);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(874);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(875);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(876);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(877);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(878);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(879);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(880);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(881);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(882);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(883);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(884);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(885);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(886);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(887);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(888);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(889);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(890);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(891);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(892);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(893);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(894);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(895);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(896);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(897);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(898);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(899);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(900);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(901);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(902);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(903);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(904);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(905);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(906);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(907);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(908);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(909);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(910);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(911);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(912);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(913);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(914);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(915);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(916);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(917);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(918);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(919);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(920);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(921);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(922);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(923);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(924);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(925);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(926);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(927);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(928);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(929);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(930);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(931);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(932);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(933);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(934);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(935);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(936);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(937);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(938);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(939);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(940);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(941);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(942);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(943);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(944);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(945);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(946);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(947);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(948);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(949);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(950);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(951);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(952);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(953);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(954);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(955);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(956);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(957);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(958);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(959);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(960);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(961);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(962);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(963);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(964);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(965);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(966);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(967);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(968);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(969);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(970);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(971);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(972);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(973);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint16 op_$equals(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(974);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(975);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_uint16 parse(object s) {
		pushObj(s);
		call(976);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_ulong(object $instance) {
		pushObj($instance);
		call(977);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_long(object $instance) {
		pushObj($instance);
		call(978);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uint(object $instance) {
		pushObj($instance);
		call(979);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_int(object $instance) {
		pushObj($instance);
		call(980);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_ushort(object $instance) {
		pushObj($instance);
		call(981);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_short(object $instance) {
		pushObj($instance);
		call(982);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uchar(object $instance) {
		pushObj($instance);
		call(983);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_char(object $instance) {
		pushObj($instance);
		call(984);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_byte(object $instance) {
		pushObj($instance);
		call(985);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bool(object $instance) {
		pushObj($instance);
		call(986);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_uint16 get_value(object $instance) {
		pushObj($instance);
		call(987);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void integer2(object $instance) {
		pushObj($instance);
		call(988);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, integer$_int16$) 

	void integer(object $instance, var& initial_val) {
		pushObj($instance);
		pushNum(initial_val.value());
		call(989);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(990);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(991);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(992);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(993);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(994);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(995);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(996);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(997);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(998);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(999);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1000);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1001);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1002);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1003);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1004);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1005);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1006);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1007);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1008);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1009);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1010);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1011);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1012);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1013);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1014);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1015);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1016);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1017);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1018);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1019);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1020);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1021);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1022);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1023);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1024);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1025);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1026);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1027);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1028);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1029);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1030);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1031);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1032);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1033);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1034);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1035);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1036);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1037);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1038);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1039);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1040);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1041);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1042);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1043);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1044);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1045);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1046);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1047);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1048);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1049);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1050);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1051);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1052);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1053);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1054);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1055);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1056);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1057);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1058);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1059);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1060);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1061);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1062);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1063);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1064);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1065);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1066);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1067);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1068);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1069);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1070);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1071);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1072);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1073);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1074);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1075);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1076);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1077);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1078);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1079);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1080);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1081);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1082);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1083);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1084);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1085);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1086);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1087);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1088);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1089);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1090);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1091);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1092);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1093);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1094);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1095);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1096);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1097);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1098);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1099);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1100);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1101);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1102);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1103);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1104);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1105);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1106);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1107);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1108);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1109);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1110);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1111);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1112);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1113);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1114);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1115);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1116);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1117);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1118);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1119);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1120);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1121);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1122);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1123);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1124);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1125);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1126);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1127);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1128);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1129);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1130);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1131);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1132);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1133);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1134);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1135);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1136);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1137);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1138);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1139);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1140);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1141);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1142);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1143);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1144);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1145);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1146);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1147);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1148);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1149);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1150);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1151);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1152);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1153);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1154);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1155);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1156);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1157);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1158);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1159);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1160);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1161);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1162);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1163);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1164);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1165);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1166);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int16 op_$equals(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1167);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(1168);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int16 parse(object s) {
		pushObj(s);
		call(1169);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_ulong(object $instance) {
		pushObj($instance);
		call(1170);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_long(object $instance) {
		pushObj($instance);
		call(1171);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uint(object $instance) {
		pushObj($instance);
		call(1172);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_int(object $instance) {
		pushObj($instance);
		call(1173);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_ushort(object $instance) {
		pushObj($instance);
		call(1174);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_short(object $instance) {
		pushObj($instance);
		call(1175);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uchar(object $instance) {
		pushObj($instance);
		call(1176);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_char(object $instance) {
		pushObj($instance);
		call(1177);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_byte(object $instance) {
		pushObj($instance);
		call(1178);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bool(object $instance) {
		pushObj($instance);
		call(1179);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int16 get_value(object $instance) {
		pushObj($instance);
		call(1180);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int16 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void integer2(object $instance) {
		pushObj($instance);
		call(1181);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, integer$_uint8$) 

	void integer(object $instance, var& initial_val) {
		pushObj($instance);
		pushNum(initial_val.value());
		call(1182);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(1183);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1184);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1185);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1186);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1187);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1188);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1189);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1190);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1191);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1192);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1193);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1194);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1195);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1196);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1197);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1198);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1199);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1200);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1201);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1202);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1203);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1204);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1205);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1206);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1207);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1208);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1209);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1210);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1211);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1212);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1213);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1214);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1215);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1216);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1217);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1218);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1219);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1220);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1221);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1222);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1223);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1224);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1225);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1226);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1227);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1228);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1229);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1230);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1231);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1232);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1233);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1234);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1235);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1236);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1237);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1238);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1239);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1240);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1241);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1242);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1243);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1244);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1245);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1246);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1247);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1248);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1249);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1250);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1251);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1252);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1253);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1254);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1255);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1256);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1257);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1258);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1259);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1260);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1261);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1262);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1263);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1264);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1265);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1266);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1267);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1268);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1269);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1270);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1271);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1272);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1273);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1274);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1275);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1276);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1277);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1278);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1279);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1280);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1281);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1282);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1283);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1284);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1285);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1286);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1287);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1288);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1289);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1290);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1291);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1292);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1293);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1294);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1295);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1296);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1297);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1298);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1299);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1300);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1301);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1302);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1303);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1304);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1305);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1306);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1307);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1308);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1309);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1310);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1311);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1312);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1313);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1314);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1315);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1316);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1317);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1318);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1319);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1320);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1321);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1322);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1323);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1324);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1325);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1326);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1327);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1328);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1329);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1330);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1331);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1332);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1333);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1334);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1335);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1336);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1337);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1338);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1339);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1340);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1341);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1342);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1343);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1344);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1345);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1346);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1347);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1348);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1349);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1350);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1351);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1352);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1353);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1354);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1355);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1356);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1357);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1358);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1359);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_uint8 op_$equals(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1360);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(1361);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_uint8 parse(object s) {
		pushObj(s);
		call(1362);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_ulong(object $instance) {
		pushObj($instance);
		call(1363);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_long(object $instance) {
		pushObj($instance);
		call(1364);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uint(object $instance) {
		pushObj($instance);
		call(1365);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_int(object $instance) {
		pushObj($instance);
		call(1366);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_ushort(object $instance) {
		pushObj($instance);
		call(1367);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_short(object $instance) {
		pushObj($instance);
		call(1368);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uchar(object $instance) {
		pushObj($instance);
		call(1369);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_char(object $instance) {
		pushObj($instance);
		call(1370);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_byte(object $instance) {
		pushObj($instance);
		call(1371);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bool(object $instance) {
		pushObj($instance);
		call(1372);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_uint8 get_value(object $instance) {
		pushObj($instance);
		call(1373);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_uint8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void integer2(object $instance) {
		pushObj($instance);
		call(1374);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, integer$_int8$) 

	void integer(object $instance, var& initial_val) {
		pushObj($instance);
		pushNum(initial_val.value());
		call(1375);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object hash(object $instance) {
		pushObj($instance);
		call(1376);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var op_$not_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1377);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1378);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1379);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1380);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1381);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1382);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1383);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1384);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1385);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1386);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1387);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1388);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1389);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1390);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1391);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1392);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1393);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1394);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1395);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1396);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1397);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1398);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1399);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1400);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1401);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1402);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1403);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1404);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1405);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1406);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$plus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1407);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1408);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1409);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1410);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1411);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1412);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1413);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1414);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1415);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1416);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1417);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$minus11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1418);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1419);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1420);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1421);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1422);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1423);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1424);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1425);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1426);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1427);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1428);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mult11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1429);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1430);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1431);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1432);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1433);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1434);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1435);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1436);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1437);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1438);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1439);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$div11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1440);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1441);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1442);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1443);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1444);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1445);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1446);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1447);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1448);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1449);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1450);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$mod11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1451);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1452);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1453);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1454);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1455);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1456);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1457);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1458);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1459);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1460);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1461);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1462);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1463);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1464);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1465);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1466);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1467);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1468);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1469);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1470);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1471);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1472);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$greater_than11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1473);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1474);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1475);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1476);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1477);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1478);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1479);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1480);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1481);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1482);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1483);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$less_or_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1484);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1485);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1486);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1487);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1488);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1489);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1490);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1491);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1492);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1493);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1494);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$pow11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1495);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1496);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1497);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1498);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1499);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1500);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1501);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1502);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1503);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1504);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1505);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$left_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1506);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1507);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1508);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1509);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1510);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1511);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1512);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1513);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1514);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1515);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1516);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$right_shift11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1517);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1518);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1519);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1520);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1521);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1522);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1523);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1524);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1525);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1526);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1527);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$xor11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1528);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1529);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1530);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1531);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1532);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1533);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1534);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1535);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1536);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1537);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1538);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$and11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1539);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1540);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or2(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1541);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or3(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1542);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or4(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1543);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or5(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1544);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or6(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1545);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or7(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1546);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or8(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1547);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or9(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1548);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or10(object $instance, object num) {
		pushObj($instance);
		pushObj(num);
		call(1549);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$or11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1550);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$not_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1551);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var op_$equals_equals11(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1552);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	_int8 op_$equals(object $instance, var& num) {
		pushObj($instance);
		pushNum(num.value());
		call(1553);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(1554);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int8 parse(object s) {
		pushObj(s);
		call(1555);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_ulong(object $instance) {
		pushObj($instance);
		call(1556);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_long(object $instance) {
		pushObj($instance);
		call(1557);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uint(object $instance) {
		pushObj($instance);
		call(1558);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_int(object $instance) {
		pushObj($instance);
		call(1559);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_ushort(object $instance) {
		pushObj($instance);
		call(1560);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_short(object $instance) {
		pushObj($instance);
		call(1561);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_uchar(object $instance) {
		pushObj($instance);
		call(1562);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_char(object $instance) {
		pushObj($instance);
		call(1563);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_byte(object $instance) {
		pushObj($instance);
		call(1564);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object to_bool(object $instance) {
		pushObj($instance);
		call(1565);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	_int8 get_value(object $instance) {
		pushObj($instance);
		call(1566);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		_int8 $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void integer2(object $instance) {
		pushObj($instance);
		call(1567);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, loopable$_int8$) 

	_int8_array get_elements(object $instance) {
		pushObj($instance);
		call(1568);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		_int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	void loopable(object $instance) {
		pushObj($instance);
		call(1569);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, list$std_string$) 

	void list(object $instance) {
		pushObj($instance);
		call(3245);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list2(object $instance, var& starting_capacity) {
		pushObj($instance);
		pushNum(starting_capacity.value());
		call(3246);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list3(object $instance, object initial_data) {
		pushObj($instance);
		pushObj(initial_data);
		call(3247);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var empty(object $instance) {
		pushObj($instance);
		call(3248);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void set_compact(object $instance, var& compact_array) {
		pushObj($instance);
		pushNum(compact_array.value());
		call(3249);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_elements(object $instance) {
		pushObj($instance);
		call(3250);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$array_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3251);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3252);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object last(object $instance) {
		pushObj($instance);
		call(3253);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object put(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3254);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void expand(object $instance) {
		pushObj($instance);
		call(3255);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3256);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3257);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all2(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3258);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3259);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3260);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void pop_back(object $instance) {
		pushObj($instance);
		call(3261);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(3262);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void insert(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3263);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object find(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3264);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void remove(object $instance, object val) {
		pushObj($instance);
		pushObj(val);
		call(3265);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove2(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3266);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void replace(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3267);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3268);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof2(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3269);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3270);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object out_of_bounds_msg(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3271);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, loopable$std_string$) 

	object get_elements(object $instance) {
		pushObj($instance);
		call(3243);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void loopable(object $instance) {
		pushObj($instance);
		call(3244);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, hashtable$std_io_thread_0_std_io_task_job_controller$) 

	void hashtable(object $instance, var& initialCapacity) {
		pushObj($instance);
		pushNum(initialCapacity.value());
		call(3272);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void hashtable2(object $instance) {
		pushObj($instance);
		call(3273);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_threshold(object $instance, var& threshold) {
		pushObj($instance);
		pushNum(threshold.value());
		call(3274);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var hash(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3275);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void resize(object $instance) {
		pushObj($instance);
		call(3276);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object at(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3277);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var put(object $instance, object key, object value) {
		pushObj($instance);
		pushObj(key);
		pushObj(value);
		call(3278);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var remove(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3279);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, hashtable$std_int_0_std_io_thread$) 

	void hashtable(object $instance, var& initialCapacity) {
		pushObj($instance);
		pushNum(initialCapacity.value());
		call(3280);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void hashtable2(object $instance) {
		pushObj($instance);
		call(3281);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_threshold(object $instance, var& threshold) {
		pushObj($instance);
		pushNum(threshold.value());
		call(3282);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var hash(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3283);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void resize(object $instance) {
		pushObj($instance);
		call(3284);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object at(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3285);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var put(object $instance, object key, object value) {
		pushObj($instance);
		pushObj(key);
		pushObj(value);
		call(3286);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var remove(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3287);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, list$std_io_thread$) 

	void list(object $instance) {
		pushObj($instance);
		call(3289);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list2(object $instance, var& starting_capacity) {
		pushObj($instance);
		pushNum(starting_capacity.value());
		call(3290);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list3(object $instance, object initial_data) {
		pushObj($instance);
		pushObj(initial_data);
		call(3291);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var empty(object $instance) {
		pushObj($instance);
		call(3292);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void set_compact(object $instance, var& compact_array) {
		pushObj($instance);
		pushNum(compact_array.value());
		call(3293);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_elements(object $instance) {
		pushObj($instance);
		call(3294);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$array_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3295);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3296);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object last(object $instance) {
		pushObj($instance);
		call(3297);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object put(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3298);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void expand(object $instance) {
		pushObj($instance);
		call(3299);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3300);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3301);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all2(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3302);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3303);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3304);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void pop_back(object $instance) {
		pushObj($instance);
		call(3305);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(3306);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void insert(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3307);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object find(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3308);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void remove(object $instance, object val) {
		pushObj($instance);
		pushObj(val);
		call(3309);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove2(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3310);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void replace(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3311);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3312);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof2(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3313);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3314);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object out_of_bounds_msg(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3315);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, synced_list$std_io_task_job$) 

	void synced_list(object $instance) {
		pushObj($instance);
		call(3318);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void synced_list2(object $instance, var& starting_capacity) {
		pushObj($instance);
		pushNum(starting_capacity.value());
		call(3319);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_compact(object $instance, var& compact_array) {
		pushObj($instance);
		pushNum(compact_array.value());
		call(3320);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_elements(object $instance) {
		pushObj($instance);
		call(3321);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$array_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3322);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3323);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object last(object $instance) {
		pushObj($instance);
		call(3324);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object put(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3325);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void expand(object $instance) {
		pushObj($instance);
		call(3326);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3327);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3355);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3356);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3357);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void pop_back(object $instance) {
		pushObj($instance);
		call(3358);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(3359);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var empty(object $instance) {
		pushObj($instance);
		call(3360);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void insert(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3361);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove(object $instance, object val) {
		pushObj($instance);
		pushObj(val);
		call(3362);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove2(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3363);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3364);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof2(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3365);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object find(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3366);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var replace(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3367);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3368);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object out_of_bounds_msg(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3369);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, loopable$std_io_task_job$) 

	object get_elements(object $instance) {
		pushObj($instance);
		call(3316);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void loopable(object $instance) {
		pushObj($instance);
		call(3317);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, list$std_io_task_job$) 

	void list(object $instance) {
		pushObj($instance);
		call(3328);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list2(object $instance, var& starting_capacity) {
		pushObj($instance);
		pushNum(starting_capacity.value());
		call(3329);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list3(object $instance, object initial_data) {
		pushObj($instance);
		pushObj(initial_data);
		call(3330);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var empty(object $instance) {
		pushObj($instance);
		call(3331);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void set_compact(object $instance, var& compact_array) {
		pushObj($instance);
		pushNum(compact_array.value());
		call(3332);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_elements(object $instance) {
		pushObj($instance);
		call(3333);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$array_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3334);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3335);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object last(object $instance) {
		pushObj($instance);
		call(3336);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object put(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3337);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void expand(object $instance) {
		pushObj($instance);
		call(3338);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3339);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3340);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all2(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3341);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3342);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3343);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void pop_back(object $instance) {
		pushObj($instance);
		call(3344);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(3345);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void insert(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3346);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object find(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3347);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void remove(object $instance, object val) {
		pushObj($instance);
		pushObj(val);
		call(3348);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove2(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3349);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void replace(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3350);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3351);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof2(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3352);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3353);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object out_of_bounds_msg(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3354);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, list$var$) 

	void list(object $instance) {
		pushObj($instance);
		call(3372);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list2(object $instance, var& starting_capacity) {
		pushObj($instance);
		pushNum(starting_capacity.value());
		call(3373);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list3(object $instance, var_array& initial_data) {
		pushObj($instance);
		pushObj(initial_data.handle);
		call(3374);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var empty(object $instance) {
		pushObj($instance);
		call(3375);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void set_compact(object $instance, var& compact_array) {
		pushObj($instance);
		pushNum(compact_array.value());
		call(3376);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var_array get_elements(object $instance) {
		pushObj($instance);
		call(3377);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	var op_$array_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3378);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3379);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var last(object $instance) {
		pushObj($instance);
		call(3380);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var put(object $instance, var& index, var& element) {
		pushObj($instance);
		pushNum(index.value());
		pushNum(element.value());
		call(3381);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void expand(object $instance) {
		pushObj($instance);
		call(3382);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3383);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3384);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all2(object $instance, var_array& lst) {
		pushObj($instance);
		pushObj(lst.handle);
		call(3385);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add(object $instance, var& element) {
		pushObj($instance);
		pushNum(element.value());
		call(3386);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof(object $instance, var& element) {
		pushObj($instance);
		pushNum(element.value());
		call(3387);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void pop_back(object $instance) {
		pushObj($instance);
		call(3388);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(3389);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void insert(object $instance, var& index, var& element) {
		pushObj($instance);
		pushNum(index.value());
		pushNum(element.value());
		call(3390);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var find(object $instance, var& comparer, var& find_func) {
		pushObj($instance);
		pushNum(comparer.value());
		pushNum(find_func.value());
		call(3391);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void remove(object $instance, var& val) {
		pushObj($instance);
		pushNum(val.value());
		call(3392);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove2(object $instance, var& val, var& compare_fun) {
		pushObj($instance);
		pushNum(val.value());
		pushNum(compare_fun.value());
		call(3393);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void replace(object $instance, var& val, var& compare_fun) {
		pushObj($instance);
		pushNum(val.value());
		pushNum(compare_fun.value());
		call(3394);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3395);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof2(object $instance, var& comparer, var& find_func) {
		pushObj($instance);
		pushNum(comparer.value());
		pushNum(find_func.value());
		call(3396);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3397);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object out_of_bounds_msg(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3398);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, loopable$var$) 

	var_array get_elements(object $instance) {
		pushObj($instance);
		call(3370);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $tmpObj = getSpObjAt(0);
		var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
	}

	void loopable(object $instance) {
		pushObj($instance);
		call(3371);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, list$common_network_core_request_data_request_item$) 

	void list(object $instance) {
		pushObj($instance);
		call(3404);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list2(object $instance, var& starting_capacity) {
		pushObj($instance);
		pushNum(starting_capacity.value());
		call(3405);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list3(object $instance, object initial_data) {
		pushObj($instance);
		pushObj(initial_data);
		call(3406);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var empty(object $instance) {
		pushObj($instance);
		call(3407);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void set_compact(object $instance, var& compact_array) {
		pushObj($instance);
		pushNum(compact_array.value());
		call(3408);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_elements(object $instance) {
		pushObj($instance);
		call(3409);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$array_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3410);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3411);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object last(object $instance) {
		pushObj($instance);
		call(3412);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object put(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3413);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void expand(object $instance) {
		pushObj($instance);
		call(3414);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3415);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3416);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all2(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3417);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3418);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3419);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void pop_back(object $instance) {
		pushObj($instance);
		call(3420);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(3421);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void insert(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3422);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object find(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3423);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void remove(object $instance, object val) {
		pushObj($instance);
		pushObj(val);
		call(3424);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove2(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3425);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void replace(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3426);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3427);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof2(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3428);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3429);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object out_of_bounds_msg(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3430);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, loopable$common_network_core_request_data_request_item$) 

	object get_elements(object $instance) {
		pushObj($instance);
		call(3402);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void loopable(object $instance) {
		pushObj($instance);
		call(3403);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, hashmap$std_int_0_ui_support_drawable$) 

	void hashmap(object $instance, var& initialCapacity) {
		pushObj($instance);
		pushNum(initialCapacity.value());
		call(3434);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void hashmap2(object $instance) {
		pushObj($instance);
		call(3435);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void set_threshold(object $instance, var& threshold) {
		pushObj($instance);
		pushNum(threshold.value());
		call(3436);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var hash(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3437);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void resize(object $instance) {
		pushObj($instance);
		call(3438);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object at(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3439);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	var put(object $instance, object key, object value) {
		pushObj($instance);
		pushObj(key);
		pushObj(value);
		call(3440);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	var remove(object $instance, object key) {
		pushObj($instance);
		pushObj(key);
		call(3441);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, list$ui_support_view$) 

	void list(object $instance) {
		pushObj($instance);
		call(3444);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list2(object $instance, var& starting_capacity) {
		pushObj($instance);
		pushNum(starting_capacity.value());
		call(3445);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list3(object $instance, object initial_data) {
		pushObj($instance);
		pushObj(initial_data);
		call(3446);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var empty(object $instance) {
		pushObj($instance);
		call(3447);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void set_compact(object $instance, var& compact_array) {
		pushObj($instance);
		pushNum(compact_array.value());
		call(3448);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_elements(object $instance) {
		pushObj($instance);
		call(3449);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$array_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3450);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3451);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object last(object $instance) {
		pushObj($instance);
		call(3452);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object put(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3453);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void expand(object $instance) {
		pushObj($instance);
		call(3454);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3455);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3456);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all2(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3457);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3458);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3459);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void pop_back(object $instance) {
		pushObj($instance);
		call(3460);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(3461);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void insert(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3462);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object find(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3463);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void remove(object $instance, object val) {
		pushObj($instance);
		pushObj(val);
		call(3464);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove2(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3465);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void replace(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3466);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3467);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof2(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3468);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3469);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object out_of_bounds_msg(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3470);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, loopable$ui_support_view$) 

	object get_elements(object $instance) {
		pushObj($instance);
		call(3442);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void loopable(object $instance) {
		pushObj($instance);
		call(3443);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std, list$ui_support_fragment$) 

	void list(object $instance) {
		pushObj($instance);
		call(3475);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list2(object $instance, var& starting_capacity) {
		pushObj($instance);
		pushNum(starting_capacity.value());
		call(3476);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void list3(object $instance, object initial_data) {
		pushObj($instance);
		pushObj(initial_data);
		call(3477);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var empty(object $instance) {
		pushObj($instance);
		call(3478);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void set_compact(object $instance, var& compact_array) {
		pushObj($instance);
		pushNum(compact_array.value());
		call(3479);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object get_elements(object $instance) {
		pushObj($instance);
		call(3480);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object op_$array_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3481);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3482);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object last(object $instance) {
		pushObj($instance);
		call(3483);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object put(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3484);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void expand(object $instance) {
		pushObj($instance);
		call(3485);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void clear(object $instance) {
		pushObj($instance);
		call(3486);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3487);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add_all2(object $instance, object lst) {
		pushObj($instance);
		pushObj(lst);
		call(3488);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void add(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3489);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof(object $instance, object element) {
		pushObj($instance);
		pushObj(element);
		call(3490);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void pop_back(object $instance) {
		pushObj($instance);
		call(3491);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var size(object $instance) {
		pushObj($instance);
		call(3492);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	void insert(object $instance, var& index, object element) {
		pushObj($instance);
		pushNum(index.value());
		pushObj(element);
		call(3493);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object find(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3494);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void remove(object $instance, object val) {
		pushObj($instance);
		pushObj(val);
		call(3495);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove2(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3496);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void replace(object $instance, object val, var& compare_fun) {
		pushObj($instance);
		pushObj(val);
		pushNum(compare_fun.value());
		call(3497);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	void remove_at(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3498);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	var indexof2(object $instance, object comparer, var& find_func) {
		pushObj($instance);
		pushObj(comparer);
		pushNum(find_func.value());
		call(3499);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		var $result(getSpNumAt(0));
		decSp(1);
		return $result;
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3500);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	object out_of_bounds_msg(object $instance, var& index) {
		pushObj($instance);
		pushNum(index.value());
		call(3501);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

scope_end()

scope_begin(std, loopable$ui_support_fragment$) 

	object get_elements(object $instance) {
		pushObj($instance);
		call(3473);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void loopable(object $instance) {
		pushObj($instance);
		call(3474);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io_task, entry$std_io_thread_0_std_io_task_job_controller$) 

	void entry(object $instance, object key, object value) {
		pushObj($instance);
		pushObj(key);
		pushObj(value);
		call(3502);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3503);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void entry2(object $instance) {
		pushObj($instance);
		call(3504);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(std_io, entry$std_int_0_std_io_thread$) 

	void entry(object $instance, object key, object value) {
		pushObj($instance);
		pushObj(key);
		pushObj(value);
		call(3505);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3506);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void entry2(object $instance) {
		pushObj($instance);
		call(3507);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

scope_begin(ui_res, entry$std_int_0_ui_support_drawable$) 

	void entry(object $instance, object key, object value) {
		pushObj($instance);
		pushObj(key);
		pushObj(value);
		call(3508);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

	object to_string(object $instance) {
		pushObj($instance);
		call(3509);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}

		object $result = getSpObjAt(0);
		decSp(1);
		return $result;
	}

	void entry2(object $instance) {
		pushObj($instance);
		call(3510);

		if(internal::exceptionCheck()) {
			throw Exception(internal::getExceptionObject(), "");
		}
	}

scope_end()

