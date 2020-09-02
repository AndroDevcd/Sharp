#include "native_mapping.h"

using namespace snb_api::internal;

scope_begin(app)

    void __srt_global(object $instance) {
        pushObj($instance);
        call(0);
    }

    var main(object args) {
        pushObj(args);
        call(1558);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std)

    void __srt_global(object $instance) {
        pushObj($instance);
        call(1);
    }

    var_array snprintf(var& fmt, var& num, var& precision) {
        pushNum(fmt.value());
        pushNum(num.value());
        pushNum(precision.value());
        call(1559);

        object $tmpObj = getSpObjAt(0);
        var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    void print(_int8_array& data) {
        pushObj(data.handle);
        call(1560);
    }

    void print2(object str) {
        pushObj(str);
        call(1561);
    }

    void println(var_array& data) {
        pushObj(data.handle);
        call(1562);
    }

    void println2(_int8_array& data) {
        pushObj(data.handle);
        call(1563);
    }

    void println3(_int16_array& data) {
        pushObj(data.handle);
        call(1564);
    }

    void println4(_int32_array& data) {
        pushObj(data.handle);
        call(1565);
    }

    void println5(_int64_array& data) {
        pushObj(data.handle);
        call(1566);
    }

    void println6(_uint8_array& data) {
        pushObj(data.handle);
        call(1567);
    }

    void println7(_uint16_array& data) {
        pushObj(data.handle);
        call(1568);
    }

    void println8(_uint32_array& data) {
        pushObj(data.handle);
        call(1569);
    }

    void println9(_uint64_array& data) {
        pushObj(data.handle);
        call(1570);
    }

    void println10(var& data) {
        pushNum(data.value());
        call(1571);
    }

    void println11() {
        call(1572);
    }

    void println12(object e) {
        pushObj(e);
        call(1573);
    }

    object read_line() {
        call(1574);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object read_passwd() {
        call(1575);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var read_char() {
        call(1576);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var read_raw_char() {
        call(1577);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void println13(object str) {
        pushObj(str);
        call(1578);
    }

    void println14(object o) {
        pushObj(o);
        call(1579);
    }

    void print3(object o) {
        pushObj(o);
        call(1580);
    }

    void print4(var& data) {
        pushNum(data.value());
        call(1581);
    }

    void flush() {
        call(1582);
    }

    var utc_mills_time() {
        call(1583);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var nano_time() {
        call(1584);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void exit() {
        call(1585);
    }

    var sizeOf(object data) {
        pushObj(data);
        call(1586);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var sizeOf2(object data) {
        pushObj(data);
        call(1587);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void print_chars(_int8_array& str) {
        pushObj(str.handle);
        call(1588);
    }

    void print_num(var& val) {
        pushNum(val.value());
        call(1589);
    }

    void print_char(_int8& val) {
        pushNum(val.value());
        call(1590);
    }

    void print_numbers(var_array& str) {
        pushObj(str.handle);
        call(1591);
    }

    void print_numbers2(_int16_array& str) {
        pushObj(str.handle);
        call(1592);
    }

    void print_numbers3(_int32_array& str) {
        pushObj(str.handle);
        call(1593);
    }

    void print_numbers4(_int64_array& str) {
        pushObj(str.handle);
        call(1594);
    }

    void print_numbers5(_uint8_array& str) {
        pushObj(str.handle);
        call(1595);
    }

    void print_numbers6(_uint16_array& str) {
        pushObj(str.handle);
        call(1596);
    }

    void print_numbers7(_uint32_array& str) {
        pushObj(str.handle);
        call(1597);
    }

    void print_numbers8(_uint64_array& str) {
        pushObj(str.handle);
        call(1598);
    }

    void print_chars2(_int16_array& str) {
        pushObj(str.handle);
        call(1599);
    }

    var read_ch(var& hide) {
        pushNum(hide.value());
        call(1600);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object runtime_error(object message) {
        pushObj(message);
        call(1657);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object runtime_error2() {
        call(1658);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void todo(object message) {
        pushObj(message);
        call(1659);
    }

    void $03internal_static_init() {
        call(3191);
    }

scope_end()

scope_begin(std, _enum_)

    void _enum_(object $instance, var& ord) {
        pushObj($instance);
        pushNum(ord.value());
        call(1601);
    }

    void _enum_2(object $instance, object e) {
        pushObj($instance);
        pushObj(e);
        call(1602);
    }

    var op_$not_equals(object $instance, object e) {
        pushObj($instance);
        pushObj(e);
        call(1603);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1604);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object e) {
        pushObj($instance);
        pushObj(e);
        call(1605);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1606);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8_array get_name(object $instance) {
        pushObj($instance);
        call(1607);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int64 get_ordinal(object $instance) {
        pushObj($instance);
        call(1608);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(1609);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void _enum_3(object $instance) {
        pushObj($instance);
        call(1610);
    }

scope_end()

scope_begin(std, throwable)

    void throwable(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1611);
    }

    void throwable2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1612);
    }

    object get_message(object $instance) {
        pushObj($instance);
        call(1613);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void throwable3(object $instance) {
        pushObj($instance);
        call(1614);
    }

    object get_stack_trace(object $instance) {
        pushObj($instance);
        call(3073);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, exception)

    void exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1615);
    }

    void exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1616);
    }

    void exception3(object $instance) {
        pushObj($instance);
        call(1617);
    }

scope_end()

scope_begin(std, illegal_argument_exception)

    void illegal_argument_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1618);
    }

    void illegal_argument_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1619);
    }

    void illegal_argument_exception3(object $instance) {
        pushObj($instance);
        call(1620);
    }

scope_end()

scope_begin(std, illegal_state_exception)

    void illegal_state_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1621);
    }

    void illegal_state_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1622);
    }

    void illegal_state_exception3(object $instance) {
        pushObj($instance);
        call(1623);
    }

scope_end()

scope_begin(std, out_of_bounds_exception)

    void out_of_bounds_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1624);
    }

    void out_of_bounds_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1625);
    }

    void out_of_bounds_exception3(object $instance) {
        pushObj($instance);
        call(1626);
    }

scope_end()

scope_begin(std, io_exception)

    void io_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1627);
    }

    void io_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1628);
    }

    void io_exception3(object $instance) {
        pushObj($instance);
        call(1629);
    }

scope_end()

scope_begin(std, invalid_operation_exception)

    void invalid_operation_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1630);
    }

    void invalid_operation_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1631);
    }

    void invalid_operation_exception3(object $instance) {
        pushObj($instance);
        call(1632);
    }

scope_end()

scope_begin(std, nullptr_exception)

    void nullptr_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1633);
    }

    void nullptr_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1634);
    }

    void nullptr_exception3(object $instance) {
        pushObj($instance);
        call(1635);
    }

scope_end()

scope_begin(std, runtime_exception)

    void runtime_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1636);
    }

    void runtime_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1637);
    }

    void runtime_exception3(object $instance) {
        pushObj($instance);
        call(1638);
    }

scope_end()

scope_begin(std, stack_overflow_exception)

    void stack_overflow_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1639);
    }

    void stack_overflow_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1640);
    }

    void stack_overflow_exception3(object $instance) {
        pushObj($instance);
        call(1641);
    }

scope_end()

scope_begin(std, thread_stack_exception)

    void thread_stack_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1642);
    }

    void thread_stack_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1643);
    }

    void thread_stack_exception3(object $instance) {
        pushObj($instance);
        call(1644);
    }

scope_end()

scope_begin(std, class_cast_exception)

    void class_cast_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1645);
    }

    void class_cast_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1646);
    }

    void class_cast_exception3(object $instance) {
        pushObj($instance);
        call(1647);
    }

scope_end()

scope_begin(std, out_of_memory_exception)

    void out_of_memory_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1648);
    }

    void out_of_memory_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1649);
    }

    void out_of_memory_exception3(object $instance) {
        pushObj($instance);
        call(1650);
    }

scope_end()

scope_begin(std, not_implemented_error)

    void not_implemented_error(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1651);
    }

    void not_implemented_error2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1652);
    }

    void not_implemented_error3(object $instance) {
        pushObj($instance);
        call(1653);
    }

scope_end()

scope_begin(std, unsatisfied_link_error)

    void unsatisfied_link_error(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1654);
    }

    void unsatisfied_link_error2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1655);
    }

    void unsatisfied_link_error3(object $instance) {
        pushObj($instance);
        call(1656);
    }

scope_end()

scope_begin(std_io)

    void __srt_global(object $instance) {
        pushObj($instance);
        call(2);
    }

    void critical_section(var& block) {
        pushNum(block.value());
        call(1660);
    }

    void critical_section2(var& block, object lock_obj) {
        pushNum(block.value());
        pushObj(lock_obj);
        call(1661);
    }

    void realloc(object data, var& size) {
        pushObj(data);
        pushNum(size.value());
        call(1724);
    }

    void realloc2(var_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1725);
    }

    void realloc3(_int8_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1726);
    }

    void realloc4(_int16_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1727);
    }

    void realloc5(_int32_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1728);
    }

    void realloc6(_int64_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1729);
    }

    void realloc7(_uint8_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1730);
    }

    void realloc8(_uint16_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1731);
    }

    void realloc9(_uint32_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1732);
    }

    void realloc10(_uint64_array& data, var& size) {
        pushObj(data.handle);
        pushNum(size.value());
        call(1733);
    }

    void memcopy(object src, object dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src);
        pushObj(dest);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1734);
    }

    void memcopy2(var_array& src, var_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1735);
    }

    void memcopy3(_int8_array& src, _int8_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1736);
    }

    void memcopy4(_int16_array& src, _int16_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1737);
    }

    void memcopy5(_int32_array& src, _int32_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1738);
    }

    void memcopy6(_int64_array& src, _int64_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1739);
    }

    void memcopy7(_uint8_array& src, _uint8_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1740);
    }

    void memcopy8(_uint16_array& src, _uint16_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1741);
    }

    void memcopy9(_uint32_array& src, _uint32_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1742);
    }

    void memcopy10(_uint64_array& src, _uint64_array& dest, var& destStart, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushObj(dest.handle);
        pushNum(destStart.value());
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1743);
    }

    object copy(object src, var& srcStart, var& srcEnd) {
        pushObj(src);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1744);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var_array copy2(var_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1745);

        object $tmpObj = getSpObjAt(0);
        var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int8_array copy3(_int8_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1746);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int16_array copy4(_int16_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1747);

        object $tmpObj = getSpObjAt(0);
        _int16_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int32_array copy5(_int32_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1748);

        object $tmpObj = getSpObjAt(0);
        _int32_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _uint8_array copy6(_uint8_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1749);

        object $tmpObj = getSpObjAt(0);
        _uint8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _uint16_array copy7(_uint16_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1750);

        object $tmpObj = getSpObjAt(0);
        _uint16_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _uint32_array copy8(_uint32_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1751);

        object $tmpObj = getSpObjAt(0);
        _uint32_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _uint64_array copy9(_uint64_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1752);

        object $tmpObj = getSpObjAt(0);
        _uint64_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int64_array copy10(_int64_array& src, var& srcStart, var& srcEnd) {
        pushObj(src.handle);
        pushNum(srcStart.value());
        pushNum(srcEnd.value());
        call(1753);

        object $tmpObj = getSpObjAt(0);
        _int64_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    object invert(object src, var& start, var& len) {
        pushObj(src);
        pushNum(start.value());
        pushNum(len.value());
        call(1754);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var_array invert2(var_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1755);

        object $tmpObj = getSpObjAt(0);
        var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int8_array invert3(_int8_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1756);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int16_array invert4(_int16_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1757);

        object $tmpObj = getSpObjAt(0);
        _int16_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int32_array invert5(_int32_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1758);

        object $tmpObj = getSpObjAt(0);
        _int32_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int64_array invert6(_int64_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1759);

        object $tmpObj = getSpObjAt(0);
        _int64_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _uint8_array invert7(_uint8_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1760);

        object $tmpObj = getSpObjAt(0);
        _uint8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _uint16_array invert8(_uint16_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1761);

        object $tmpObj = getSpObjAt(0);
        _uint16_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _uint32_array invert9(_uint32_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1762);

        object $tmpObj = getSpObjAt(0);
        _uint32_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _uint64_array invert10(_uint64_array& src, var& start, var& len) {
        pushObj(src.handle);
        pushNum(start.value());
        pushNum(len.value());
        call(1763);

        object $tmpObj = getSpObjAt(0);
        _uint64_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    void _srt_thread_start() {
        call(1876);
    }

    void $03internal_static_init() {
        call(3185);
    }

scope_end()

scope_begin(std_io, file)

    void file(object $instance, object path) {
        pushObj($instance);
        pushObj(path);
        call(1662);
    }

    void file2(object $instance, _int8_array& path) {
        pushObj($instance);
        pushObj(path.handle);
        call(1663);
    }

    object get_name(object $instance) {
        pushObj($instance);
        call(1664);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(1665);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void update_path(object $instance, object path) {
        pushObj($instance);
        pushObj(path);
        call(1666);
    }

    object get_parent_file(object $instance) {
        pushObj($instance);
        call(1667);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object get_path(object $instance) {
        pushObj($instance);
        call(1668);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object get_absolute_path(object $instance) {
        pushObj($instance);
        call(1669);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var is_readable(object $instance) {
        pushObj($instance);
        call(1670);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_writeable(object $instance) {
        pushObj($instance);
        call(1671);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var exists(object $instance) {
        pushObj($instance);
        call(1672);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_directory(object $instance) {
        pushObj($instance);
        call(1673);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_file(object $instance) {
        pushObj($instance);
        call(1674);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_hidden(object $instance) {
        pushObj($instance);
        call(1675);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var last_modified(object $instance) {
        pushObj($instance);
        call(1676);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var last_status_changed(object $instance) {
        pushObj($instance);
        call(1677);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var last_accessed(object $instance) {
        pushObj($instance);
        call(1678);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var length(object $instance) {
        pushObj($instance);
        call(1679);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void create(object $instance) {
        pushObj($instance);
        call(1680);
    }

    var _delete(object $instance) {
        pushObj($instance);
        call(1681);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object list(object $instance) {
        pushObj($instance);
        call(1682);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var mkdir(object $instance) {
        pushObj($instance);
        call(1683);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var rename(object $instance, object dest) {
        pushObj($instance);
        pushObj(dest);
        call(1684);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_last_modified(object $instance, var& time) {
        pushObj($instance);
        pushNum(time.value());
        call(1685);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_read_only(object $instance) {
        pushObj($instance);
        call(1686);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_writable(object $instance, var& writable, var& owner_only) {
        pushObj($instance);
        pushNum(writable.value());
        pushNum(owner_only.value());
        call(1687);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_writable2(object $instance, var& writable) {
        pushObj($instance);
        pushNum(writable.value());
        call(1688);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_readable(object $instance, var& readable, var& owner_only) {
        pushObj($instance);
        pushNum(readable.value());
        pushNum(owner_only.value());
        call(1689);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_readable2(object $instance, var& readable) {
        pushObj($instance);
        pushNum(readable.value());
        call(1690);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_executable(object $instance, var& executable, var& owner_only) {
        pushObj($instance);
        pushNum(executable.value());
        pushNum(owner_only.value());
        call(1691);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_executable2(object $instance, var& executable) {
        pushObj($instance);
        pushNum(executable.value());
        call(1692);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_executable(object $instance) {
        pushObj($instance);
        call(1693);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var get_total_space(object $instance) {
        pushObj($instance);
        call(1694);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var get_free_space(object $instance) {
        pushObj($instance);
        call(1695);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var get_usable_space(object $instance) {
        pushObj($instance);
        call(1696);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object generate_temp_file(object prefix, object suffix, object dir) {
        pushObj(prefix);
        pushObj(suffix);
        pushObj(dir);
        call(1697);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object create_temp_file(object prefix, object suffix, object directory) {
        pushObj(prefix);
        pushObj(suffix);
        pushObj(directory);
        call(1698);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object create_temp_file2(object prefix, object suffix) {
        pushObj(prefix);
        pushObj(suffix);
        call(1699);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object read_all_text(object $instance) {
        pushObj($instance);
        call(1700);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var write(object $instance, object data) {
        pushObj($instance);
        pushObj(data);
        call(1701);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object get_file_system() {
        call(1702);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void file3(object $instance) {
        pushObj($instance);
        call(1703);
    }

    void $03internal_static_init() {
        call(3186);
    }

scope_end()

scope_begin(std_io, file_system)

    var path_separator(object $instance) {
        pushObj($instance);
        call(1704);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object normalize(object $instance, object path) {
        pushObj($instance);
        pushObj(path);
        call(1705);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object resolve(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1706);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var get_attributes(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1707);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var check_access(object $instance, object f, var& access) {
        pushObj($instance);
        pushObj(f);
        pushNum(access.value());
        call(1708);

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
        call(1709);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var last_modified_time(object $instance, object f, var& stat_id) {
        pushObj($instance);
        pushObj(f);
        pushNum(stat_id.value());
        call(1710);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var get_length(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1711);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void create_file(object $instance, object pathname) {
        pushObj($instance);
        pushObj(pathname);
        call(1712);
    }

    var delete_directory(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1713);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var _delete(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1714);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object list(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1715);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var create_directory(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1716);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var rename(object $instance, object f1, object f2) {
        pushObj($instance);
        pushObj(f1);
        pushObj(f2);
        call(1717);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object read(object f) {
        pushObj(f);
        call(1718);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var write(object f, object data) {
        pushObj(f);
        pushObj(data);
        call(1719);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_last_modified_time(object $instance, object f, var& time) {
        pushObj($instance);
        pushObj(f);
        pushNum(time.value());
        call(1720);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_read_only(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1721);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var get_space_available(var& request) {
        pushNum(request.value());
        call(1722);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void file_system(object $instance) {
        pushObj($instance);
        call(1723);
    }

scope_end()

scope_begin(std_io_task)

    void __srt_global(object $instance) {
        pushObj($instance);
        call(3);
    }

    void start_scheduler() {
        call(1809);
    }

    void calculate_max_threads() {
        call(1810);
    }

    void set_thread_max(var& max) {
        pushNum(max.value());
        call(1811);
    }

    var get_thread_max() {
        call(1812);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var scheduler_main(object args) {
        pushObj(args);
        call(1813);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object get_host(var& long_running) {
        pushNum(long_running.value());
        call(1814);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void execute_synchronous(object scheduled_job) {
        pushObj(scheduled_job);
        call(1815);
    }

    var execute_job(object scheduled_job) {
        pushObj(scheduled_job);
        call(1816);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void finish() {
        call(1817);
    }

    void $03internal_static_init() {
        call(3188);
    }

    var anon_func$3196(object it, object it2) {
        pushObj(it);
        pushObj(it2);
        call(3196);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var anon_func$3197(object it, object it2) {
        pushObj(it);
        pushObj(it2);
        call(3197);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var anon_func$3198(object it, object it2) {
        pushObj(it);
        pushObj(it2);
        call(3198);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var anon_func$3199(object t) {
        pushObj(t);
        call(3199);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std_io_task, cancellation_exception)

    void cancellation_exception(object $instance, _int8_array& message) {
        pushObj($instance);
        pushObj(message.handle);
        call(1764);
    }

    void cancellation_exception2(object $instance, object message) {
        pushObj($instance);
        pushObj(message);
        call(1765);
    }

    void cancellation_exception3(object $instance) {
        pushObj($instance);
        call(1766);
    }

scope_end()

scope_begin(std_io_task, job)

    void job(object $instance) {
        pushObj($instance);
        call(1767);
    }

    void job2(object $instance, object name) {
        pushObj($instance);
        pushObj(name);
        call(1768);
    }

    var execute_async(object $instance) {
        pushObj($instance);
        call(1769);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void cancel(object $instance) {
        pushObj($instance);
        call(1770);
    }

    void cancel_future(object $instance) {
        pushObj($instance);
        call(1771);
    }

    void join(object $instance) {
        pushObj($instance);
        call(1772);
    }

    object get_state(object $instance) {
        pushObj($instance);
        call(1773);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object get_name(object $instance) {
        pushObj($instance);
        call(1774);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var get_id(object $instance) {
        pushObj($instance);
        call(1775);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void op_$equals(object $instance, var& block) {
        pushObj($instance);
        pushNum(block.value());
        call(1776);
    }

    void op_$equals2(object $instance, var& block) {
        pushObj($instance);
        pushNum(block.value());
        call(1777);
    }

    void op_$equals3(object $instance, var& block) {
        pushObj($instance);
        pushNum(block.value());
        call(1778);
    }

    void op_$equals4(object $instance, var& block) {
        pushObj($instance);
        pushNum(block.value());
        call(1779);
    }

    var job_main(object args) {
        pushObj(args);
        call(1780);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void execute(object $instance) {
        pushObj($instance);
        call(1781);
    }

    var is_finished(object $instance) {
        pushObj($instance);
        call(1782);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_active(object $instance) {
        pushObj($instance);
        call(1783);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void update_timeout(object $instance, var& tm) {
        pushObj($instance);
        pushNum(tm.value());
        call(1784);
    }

    void update_interval(object $instance, var& tm, object interval) {
        pushObj($instance);
        pushNum(tm.value());
        pushObj(interval);
        call(1785);
    }

    void reschedule(object $instance) {
        pushObj($instance);
        call(1786);
    }

    void $03internal_static_init() {
        call(3187);
    }

scope_end()

scope_begin(std_io_task, job_builder)

    void job_builder(object $instance) {
        pushObj($instance);
        call(1787);
    }

    void job_builder2(object $instance, object scheduled_job, object master) {
        pushObj($instance);
        pushObj(scheduled_job);
        pushObj(master);
        call(1788);
    }

    object with_timeout(object $instance, var& mills_time) {
        pushObj($instance);
        pushNum(mills_time.value());
        call(1789);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object schedule(object $instance, var& future) {
        pushObj($instance);
        pushNum(future.value());
        call(1790);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object recurring(object $instance, var& tm, object interval) {
        pushObj($instance);
        pushNum(tm.value());
        pushObj(interval);
        call(1791);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object with_name(object $instance, object job_name) {
        pushObj($instance);
        pushObj(job_name);
        call(1792);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object with_args(object $instance, object args) {
        pushObj($instance);
        pushObj(args);
        call(1793);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object block(object $instance) {
        pushObj($instance);
        call(1794);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object long_term(object $instance) {
        pushObj($instance);
        call(1795);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$equals(object $instance, var& main) {
        pushObj($instance);
        pushNum(main.value());
        call(1796);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$equals2(object $instance, var& main) {
        pushObj($instance);
        pushNum(main.value());
        call(1797);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$equals3(object $instance, var& main) {
        pushObj($instance);
        pushNum(main.value());
        call(1798);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$equals4(object $instance, var& main) {
        pushObj($instance);
        pushNum(main.value());
        call(1799);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object get_job(object $instance) {
        pushObj($instance);
        call(1800);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std_io_task, job_controller)

    void job_controller(object $instance) {
        pushObj($instance);
        call(1801);
    }

scope_end()

scope_begin(std_io_task, job_master)

    void job_master(object $instance) {
        pushObj($instance);
        call(1802);
    }

    object op_$equals(object $instance, var& main) {
        pushObj($instance);
        pushNum(main.value());
        call(1803);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object create_job(object $instance) {
        pushObj($instance);
        call(1804);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object create_job2(object $instance, object name) {
        pushObj($instance);
        pushObj(name);
        call(1805);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void start_job(object $instance, object scheduled_job) {
        pushObj($instance);
        pushObj(scheduled_job);
        call(1806);
    }

    object get_host_controller(object $instance, object host) {
        pushObj($instance);
        pushObj(host);
        call(1807);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void cleanup_job(object $instance, object scheduled_job) {
        pushObj($instance);
        pushObj(scheduled_job);
        call(1808);
    }

scope_end()

scope_begin(std_io_task, task)

    object with_timeout(var& mills_time) {
        pushNum(mills_time.value());
        call(1818);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object schedule(var& future) {
        pushNum(future.value());
        call(1819);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object with_name(object job_name) {
        pushObj(job_name);
        call(1820);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object with_args(object args) {
        pushObj(args);
        call(1821);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object block() {
        call(1822);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object long_term() {
        call(1823);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object builder() {
        call(1824);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void task(object $instance) {
        pushObj($instance);
        call(1825);
    }

scope_end()

scope_begin(std_io, thread)

    void thread(object $instance) {
        pushObj($instance);
        call(1826);
    }

    void thread2(object $instance, object name, var& main) {
        pushObj($instance);
        pushObj(name);
        pushNum(main.value());
        call(1827);
    }

    void thread3(object $instance, object name, var& daemon, var& main) {
        pushObj($instance);
        pushObj(name);
        pushNum(daemon.value());
        pushNum(main.value());
        call(1828);
    }

    void thread4(object $instance, object name, var& main, var& stack_size) {
        pushObj($instance);
        pushObj(name);
        pushNum(main.value());
        pushNum(stack_size.value());
        call(1829);
    }

    void thread5(object $instance, object name, var& daemon, var& main, var& stack_size) {
        pushObj($instance);
        pushObj(name);
        pushNum(daemon.value());
        pushNum(main.value());
        pushNum(stack_size.value());
        call(1830);
    }

    void thread6(object $instance, object name, var& main, var& stack_size, object group) {
        pushObj($instance);
        pushObj(name);
        pushNum(main.value());
        pushNum(stack_size.value());
        pushObj(group);
        call(1831);
    }

    void thread7(object $instance, object name, var& main, object group) {
        pushObj($instance);
        pushObj(name);
        pushNum(main.value());
        pushObj(group);
        call(1832);
    }

    void thread8(object $instance, object name, var& daemon, var& main, var& stack_size, object group) {
        pushObj($instance);
        pushObj(name);
        pushNum(daemon.value());
        pushNum(main.value());
        pushNum(stack_size.value());
        pushObj(group);
        call(1833);
    }

    object start(object $instance, object args) {
        pushObj($instance);
        pushObj(args);
        call(1834);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object start2(object $instance) {
        pushObj($instance);
        call(1835);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object suspend(object $instance) {
        pushObj($instance);
        call(1836);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object unsuspend(object $instance) {
        pushObj($instance);
        call(1837);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object unsuspend_and_wait(object $instance) {
        pushObj($instance);
        call(1838);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object join(object $instance) {
        pushObj($instance);
        call(1839);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object interrupt(object $instance) {
        pushObj($instance);
        call(1840);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object current() {
        call(1841);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void sleep(var& mills_time) {
        pushNum(mills_time.value());
        call(1842);
    }

    void wait() {
        call(1843);
    }

    void wait2(var& mills_time) {
        pushNum(mills_time.value());
        call(1844);
    }

    void exit(var& code) {
        pushNum(code.value());
        call(1845);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(1846);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void yield() {
        call(1847);
    }

    void set_exception_handler(object $instance, object error_handler) {
        pushObj($instance);
        pushObj(error_handler);
        call(1848);
    }

    object set_priority(object $instance, object priority) {
        pushObj($instance);
        pushObj(priority);
        call(1849);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void remove_thread(object $instance, object t) {
        pushObj($instance);
        pushObj(t);
        call(1850);
    }

    var get_id(object $instance) {
        pushObj($instance);
        call(1851);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var get_native_handle(object $instance) {
        pushObj($instance);
        call(1852);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var get_stack_size(object $instance) {
        pushObj($instance);
        call(1853);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object get_name(object $instance) {
        pushObj($instance);
        call(1854);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var is_exited(object $instance) {
        pushObj($instance);
        call(1855);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_daemon(object $instance) {
        pushObj($instance);
        call(1856);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_started(object $instance) {
        pushObj($instance);
        call(1857);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_suspended(object $instance) {
        pushObj($instance);
        call(1858);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var set_main(object $instance, var& main) {
        pushObj($instance);
        pushNum(main.value());
        call(1859);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_active(object $instance) {
        pushObj($instance);
        call(1860);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var get_exit_code(object $instance) {
        pushObj($instance);
        call(1861);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object get_exception_handler(object $instance) {
        pushObj($instance);
        call(1862);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object t) {
        pushObj($instance);
        pushObj(t);
        call(1863);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std_io, thread_group)

    void thread_group(object $instance, object name) {
        pushObj($instance);
        pushObj(name);
        call(1864);
    }

    void thread_group2(object $instance, object name, object priority) {
        pushObj($instance);
        pushObj(name);
        pushObj(priority);
        call(1865);
    }

    object at(object $instance, var& id) {
        pushObj($instance);
        pushNum(id.value());
        call(1866);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void add(object $instance, object t) {
        pushObj($instance);
        pushObj(t);
        call(1867);
    }

    var size(object $instance) {
        pushObj($instance);
        call(1868);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object get_name(object $instance) {
        pushObj($instance);
        call(1869);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object get_priority(object $instance) {
        pushObj($instance);
        call(1870);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var remove(object $instance, var& id) {
        pushObj($instance);
        pushNum(id.value());
        call(1871);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object get_elements(object $instance) {
        pushObj($instance);
        call(1872);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object find(object $instance, var& find_func) {
        pushObj($instance);
        pushNum(find_func.value());
        call(1873);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void kill_all(object $instance) {
        pushObj($instance);
        call(1874);
    }

    void thread_group3(object $instance) {
        pushObj($instance);
        call(1875);
    }

    void $03internal_static_init() {
        call(3189);
    }

scope_end()

scope_begin(std_io, thread_result)

    object at(var& index) {
        pushNum(index.value());
        call(1877);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void thread_result(object $instance) {
        pushObj($instance);
        call(1878);
    }

    void $03internal_static_init() {
        call(3195);
    }

scope_end()

scope_begin(std_io, uncaught_exception_handler)

    void uncaught_exception(object $instance, object err) {
        pushObj($instance);
        pushObj(err);
        call(1879);
    }

    void uncaught_exception_handler(object $instance) {
        pushObj($instance);
        call(1880);
    }

scope_end()

scope_begin(std_io, unix_fs)

    var path_separator(object $instance) {
        pushObj($instance);
        call(1881);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object normalize(object $instance, object pathname) {
        pushObj($instance);
        pushObj(pathname);
        call(1882);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var get_attributes(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1883);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var check_access(object $instance, object f, var& access) {
        pushObj($instance);
        pushObj(f);
        pushNum(access.value());
        call(1884);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void unix_fs(object $instance) {
        pushObj($instance);
        call(1885);
    }

scope_end()

scope_begin(std_io, win_fs)

    var path_separator(object $instance) {
        pushObj($instance);
        call(1886);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object normalize(object $instance, object pathname) {
        pushObj($instance);
        pushObj(pathname);
        call(1887);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var get_attributes(object $instance, object f) {
        pushObj($instance);
        pushObj(f);
        call(1888);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var check_access(object $instance, object f, var& access) {
        pushObj($instance);
        pushObj(f);
        pushNum(access.value());
        call(1889);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void win_fs(object $instance) {
        pushObj($instance);
        call(1890);
    }

scope_end()

scope_begin(std_math)

    void __srt_global(object $instance) {
        pushObj($instance);
        call(4);
    }

    var is_nan(var& v) {
        pushNum(v.value());
        call(1891);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_infinite(var& v) {
        pushNum(v.value());
        call(1892);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var sin(var& num) {
        pushNum(num.value());
        call(1893);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var cos(var& num) {
        pushNum(num.value());
        call(1894);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var tan(var& num) {
        pushNum(num.value());
        call(1895);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var sinh(var& num) {
        pushNum(num.value());
        call(1896);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var cosh(var& num) {
        pushNum(num.value());
        call(1897);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var tanh(var& num) {
        pushNum(num.value());
        call(1898);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var asin(var& num) {
        pushNum(num.value());
        call(1899);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var acos(var& num) {
        pushNum(num.value());
        call(1900);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var atan(var& num) {
        pushNum(num.value());
        call(1901);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var atan2(var& num1, var& num2) {
        pushNum(num1.value());
        pushNum(num2.value());
        call(1902);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var exp(var& num) {
        pushNum(num.value());
        call(1903);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var log(var& num) {
        pushNum(num.value());
        call(1904);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var log10(var& num) {
        pushNum(num.value());
        call(1905);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var pow(var& num, var& exp) {
        pushNum(num.value());
        pushNum(exp.value());
        call(1906);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var sqrt(var& num) {
        pushNum(num.value());
        call(1907);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var ceil(var& num) {
        pushNum(num.value());
        call(1908);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var floor(var& num) {
        pushNum(num.value());
        call(1909);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var abs(var& num) {
        pushNum(num.value());
        call(1910);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var fabs(var& num) {
        pushNum(num.value());
        call(1911);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var ldexp(var& num, var& num2) {
        pushNum(num.value());
        pushNum(num2.value());
        call(1912);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var fmod(var& num, var& num2) {
        pushNum(num.value());
        pushNum(num2.value());
        call(1913);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var round(var& num) {
        pushNum(num.value());
        call(1914);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var math_proc(var& num, var& proc) {
        pushNum(num.value());
        pushNum(proc.value());
        call(1915);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var math_proc2(var& num, var& num2, var& proc) {
        pushNum(num.value());
        pushNum(num2.value());
        pushNum(proc.value());
        call(1916);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, _nil_)

    object to_string(object $instance) {
        pushObj($instance);
        call(1917);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void _nil_(object $instance) {
        pushObj($instance);
        call(1918);
    }

    void set_instance(object value) {
        pushObj(value);
        call(3176);
    }

    void $03internal_static_init() {
        call(3190);
    }

scope_end()

scope_begin(std, _object_)

    object to_string(object $instance) {
        pushObj($instance);
        call(1919);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void wait(object $instance) {
        pushObj($instance);
        call(1920);
    }

    void notify(object $instance) {
        pushObj($instance);
        call(1921);
    }

    void notify2(object $instance, var& mills) {
        pushObj($instance);
        pushNum(mills.value());
        call(1922);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(1923);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object guid(object $instance) {
        pushObj($instance);
        call(1924);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void _object_(object $instance) {
        pushObj($instance);
        call(1925);
    }

scope_end()

scope_begin(platform)

    void __srt_global(object $instance) {
        pushObj($instance);
        call(5);
    }

    void load_library(object name) {
        pushObj(name);
        call(1929);
    }

    var free_library(object name) {
        pushObj(name);
        call(1930);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var link(object name, object library) {
        pushObj(name);
        pushObj(library);
        call(1931);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(platform, build)

    var is_windows() {
        call(1926);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_linux() {
        call(1927);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void build(object $instance) {
        pushObj($instance);
        call(1928);
    }

scope_end()

scope_begin(platform_kernel)

    void __srt_global(object $instance) {
        pushObj($instance);
        call(6);
    }

scope_end()

scope_begin(platform_kernel, platform)

    void tls_init() {
        call(1932);
    }

    void static_init() {
        call(1933);
    }

    void exit(var& value) {
        pushNum(value.value());
        call(1934);
    }

    _int8_array get_lib_path(object name) {
        pushObj(name);
        call(1935);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    void load_library(object name) {
        pushObj(name);
        call(1936);
    }

    var free_library(object name) {
        pushObj(name);
        call(1937);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var link_func(object name, object library) {
        pushObj(name);
        pushObj(library);
        call(1938);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var srt_init(object args) {
        pushObj(args);
        call(1939);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void platform(object $instance) {
        pushObj($instance);
        call(1940);
    }

scope_end()

scope_begin(platform_kernel, stack_state)

    void stack_state(object $instance) {
        pushObj($instance);
        call(1941);
    }

scope_end()

scope_begin(platform_kernel, vm)

    object get_frame_info() {
        call(1942);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object get_stack_trace(object info) {
        pushObj(info);
        call(1943);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var thread_create(var& daemon) {
        pushNum(daemon.value());
        call(1944);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object suspend(object t) {
        pushObj(t);
        call(1945);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object unsuspend(object t, var& wait) {
        pushObj(t);
        pushNum(wait.value());
        call(1946);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object thread_join(object t) {
        pushObj(t);
        call(1947);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object kill(object t) {
        pushObj(t);
        call(1948);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object current_thread() {
        call(1949);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void exit_thread(object t, var& code) {
        pushObj(t);
        pushNum(code.value());
        call(1950);
    }

    void thread_yield() {
        call(1951);
    }

    object thread_start(object t, object args) {
        pushObj(t);
        pushObj(args);
        call(1952);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object set_thread_priority(object t, var& priority) {
        pushObj(t);
        pushNum(priority.value());
        call(1953);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void set_thread(object t) {
        pushObj(t);
        call(1954);
    }

    object thread_args() {
        call(1955);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var cores() {
        call(1956);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void vm(object $instance) {
        pushObj($instance);
        call(1957);
    }

scope_end()

scope_begin(std, ulong)

    void ulong(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1958);
    }

    void ulong2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1959);
    }

    void ulong3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1960);
    }

    void ulong4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1961);
    }

    void ulong5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1962);
    }

    void ulong6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1963);
    }

    void ulong7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1964);
    }

    void ulong8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1965);
    }

    void ulong9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1966);
    }

    void ulong10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(1967);
    }

    void ulong11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(1968);
    }

    _uint64 op_$not(object $instance) {
        pushObj($instance);
        call(1969);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(1970);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1971);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(1972);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1973);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1974);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1975);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1976);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1977);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1978);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1979);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1980);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1981);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1982);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1983);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1984);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1985);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1986);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1987);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1988);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1989);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1990);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1991);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1992);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1993);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1994);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1995);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1996);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1997);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1998);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1999);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2000);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2001);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2002);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2003);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2004);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2005);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2006);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2007);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2008);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2009);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2010);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2011);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2012);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2013);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2014);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2015);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2016);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2017);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2018);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2019);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2020);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2021);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2022);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2023);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2024);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2025);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2026);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2027);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2028);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2029);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2030);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2031);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2032);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2033);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2034);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2035);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2036);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2037);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2038);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2039);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2040);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2041);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2042);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2043);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2044);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2045);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2046);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2047);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2048);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2049);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2050);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void ulong12(object $instance) {
        pushObj($instance);
        call(2051);
    }

scope_end()

scope_begin(std, _long)

    void _long(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2052);
    }

    void _long2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2053);
    }

    void _long3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2054);
    }

    void _long4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2055);
    }

    void _long5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2056);
    }

    void _long6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2057);
    }

    void _long7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2058);
    }

    void _long8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2059);
    }

    void _long9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2060);
    }

    void _long10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2061);
    }

    void _long11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2062);
    }

    _int64 op_$not(object $instance) {
        pushObj($instance);
        call(2063);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2064);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2065);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2066);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2067);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2068);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2069);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2070);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2071);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2072);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2073);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2074);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2075);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2076);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2077);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2078);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2079);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2080);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2081);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2082);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2083);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2084);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2085);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2086);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2087);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2088);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2089);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2090);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2091);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2092);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2093);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2094);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2095);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2096);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2097);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2098);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2099);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2100);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2101);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2102);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2103);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2104);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2105);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2106);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2107);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2108);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2109);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2110);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2111);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2112);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2113);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2114);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2115);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2116);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2117);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2118);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2119);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2120);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2121);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2122);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2123);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2124);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2125);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2126);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2127);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2128);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2129);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2130);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2131);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2132);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2133);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2134);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2135);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2136);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2137);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2138);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2139);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2140);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2141);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2142);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2143);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2144);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void _long12(object $instance) {
        pushObj($instance);
        call(2145);
    }

scope_end()

scope_begin(std, uint)

    void uint(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2146);
    }

    void uint2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2147);
    }

    void uint3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2148);
    }

    void uint4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2149);
    }

    void uint5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2150);
    }

    void uint6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2151);
    }

    void uint7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2152);
    }

    void uint8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2153);
    }

    void uint9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2154);
    }

    void uint10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2155);
    }

    void uint11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2156);
    }

    _uint32 op_$not(object $instance) {
        pushObj($instance);
        call(2157);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2158);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2159);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2160);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2161);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2162);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2163);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2164);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2165);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2166);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2167);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2168);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2169);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2170);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2171);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2172);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2173);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2174);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2175);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2176);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2177);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2178);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2179);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2180);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2181);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2182);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2183);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2184);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2185);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2186);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2187);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2188);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2189);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2190);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2191);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2192);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2193);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2194);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2195);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2196);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2197);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2198);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2199);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2200);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2201);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2202);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2203);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2204);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2205);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2206);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2207);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2208);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2209);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2210);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2211);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2212);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2213);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2214);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2215);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2216);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2217);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2218);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2219);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2220);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2221);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2222);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2223);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2224);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2225);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2226);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2227);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2228);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2229);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2230);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2231);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2232);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2233);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2234);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2235);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2236);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2237);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2238);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void uint12(object $instance) {
        pushObj($instance);
        call(2239);
    }

scope_end()

scope_begin(std, _int)

    void _int(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2240);
    }

    void _int2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2241);
    }

    void _int3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2242);
    }

    void _int4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2243);
    }

    void _int5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2244);
    }

    void _int6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2245);
    }

    void _int7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2246);
    }

    void _int8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2247);
    }

    void _int9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2248);
    }

    void _int10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2249);
    }

    void _int11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2250);
    }

    _int32 op_$not(object $instance) {
        pushObj($instance);
        call(2251);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2252);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2253);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2254);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2255);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2256);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2257);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2258);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2259);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2260);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2261);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2262);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2263);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2264);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2265);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2266);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2267);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2268);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2269);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2270);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2271);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2272);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2273);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2274);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2275);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2276);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2277);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2278);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2279);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2280);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2281);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2282);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2283);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2284);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2285);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2286);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2287);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2288);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2289);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2290);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2291);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2292);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2293);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2294);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2295);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2296);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2297);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2298);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2299);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2300);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2301);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2302);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2303);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2304);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2305);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2306);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2307);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2308);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2309);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2310);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2311);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2312);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2313);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2314);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2315);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2316);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2317);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2318);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2319);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2320);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2321);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2322);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2323);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2324);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2325);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2326);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2327);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2328);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2329);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2330);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2331);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2332);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void _int12(object $instance) {
        pushObj($instance);
        call(2333);
    }

scope_end()

scope_begin(std, ushort)

    void ushort(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2334);
    }

    void ushort2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2335);
    }

    void ushort3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2336);
    }

    void ushort4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2337);
    }

    void ushort5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2338);
    }

    void ushort6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2339);
    }

    void ushort7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2340);
    }

    void ushort8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2341);
    }

    void ushort9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2342);
    }

    void ushort10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2343);
    }

    void ushort11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2344);
    }

    _uint16 op_$not(object $instance) {
        pushObj($instance);
        call(2345);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2346);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2347);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2348);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2349);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2350);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2351);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2352);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2353);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2354);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2355);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2356);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2357);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2358);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2359);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2360);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2361);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2362);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2363);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2364);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2365);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2366);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2367);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2368);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2369);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2370);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2371);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2372);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2373);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2374);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2375);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2376);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2377);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2378);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2379);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2380);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2381);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2382);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2383);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2384);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2385);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2386);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2387);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2388);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2389);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2390);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2391);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2392);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2393);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2394);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2395);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2396);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2397);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2398);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2399);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2400);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2401);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2402);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2403);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2404);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2405);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2406);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2407);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2408);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2409);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2410);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2411);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2412);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2413);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2414);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2415);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2416);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2417);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2418);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2419);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2420);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2421);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2422);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2423);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2424);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2425);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2426);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void ushort12(object $instance) {
        pushObj($instance);
        call(2427);
    }

scope_end()

scope_begin(std, _short)

    void _short(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2428);
    }

    void _short2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2429);
    }

    void _short3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2430);
    }

    void _short4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2431);
    }

    void _short5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2432);
    }

    void _short6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2433);
    }

    void _short7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2434);
    }

    void _short8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2435);
    }

    void _short9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2436);
    }

    void _short10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2437);
    }

    void _short11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2438);
    }

    _int16 op_$not(object $instance) {
        pushObj($instance);
        call(2439);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2440);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2441);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2442);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2443);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2444);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2445);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2446);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2447);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2448);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2449);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2450);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2451);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2452);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2453);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2454);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2455);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2456);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2457);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2458);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2459);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2460);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2461);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2462);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2463);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2464);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2465);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2466);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2467);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2468);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2469);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2470);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2471);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2472);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2473);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2474);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2475);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2476);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2477);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2478);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2479);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2480);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2481);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2482);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2483);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2484);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2485);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2486);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2487);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2488);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2489);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2490);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2491);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2492);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2493);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2494);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2495);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2496);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2497);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2498);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2499);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2500);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2501);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2502);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2503);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2504);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2505);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2506);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2507);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2508);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2509);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2510);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2511);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2512);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2513);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2514);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2515);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2516);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2517);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2518);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2519);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2520);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void _short12(object $instance) {
        pushObj($instance);
        call(2521);
    }

scope_end()

scope_begin(std, uchar)

    void uchar(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2522);
    }

    void uchar2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2523);
    }

    void uchar3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2524);
    }

    void uchar4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2525);
    }

    void uchar5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2526);
    }

    void uchar6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2527);
    }

    void uchar7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2528);
    }

    void uchar8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2529);
    }

    void uchar9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2530);
    }

    void uchar10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2531);
    }

    void uchar11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2532);
    }

    _uint8 op_$not(object $instance) {
        pushObj($instance);
        call(2533);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2534);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2535);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2536);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2537);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2538);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2539);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2540);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2541);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2542);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2543);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2544);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2545);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2546);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2547);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2548);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2549);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2550);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2551);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2552);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2553);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2554);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2555);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2556);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2557);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2558);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2559);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2560);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2561);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2562);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2563);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2564);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2565);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2566);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2567);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2568);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2569);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2570);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2571);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2572);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2573);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2574);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2575);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2576);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2577);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2578);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2579);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2580);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2581);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2582);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2583);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2584);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2585);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2586);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2587);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2588);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2589);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2590);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2591);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2592);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2593);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2594);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2595);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2596);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2597);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2598);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2599);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2600);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2601);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2602);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2603);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2604);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2605);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2606);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2607);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2608);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2609);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2610);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2611);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2612);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2613);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2614);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void uchar12(object $instance) {
        pushObj($instance);
        call(2615);
    }

scope_end()

scope_begin(std, _char)

    void _char(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2616);
    }

    void _char2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2617);
    }

    void _char3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2618);
    }

    void _char4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2619);
    }

    void _char5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2620);
    }

    void _char6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2621);
    }

    void _char7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2622);
    }

    void _char8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2623);
    }

    void _char9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2624);
    }

    void _char10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2625);
    }

    void _char11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2626);
    }

    _int8 op_$not(object $instance) {
        pushObj($instance);
        call(2627);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2628);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2629);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2630);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2631);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2632);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2633);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2634);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2635);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2636);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2637);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2638);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2639);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2640);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2641);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2642);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2643);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2644);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2645);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2646);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2647);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2648);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2649);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2650);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2651);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2652);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2653);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2654);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2655);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2656);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2657);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2658);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2659);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2660);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2661);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2662);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2663);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2664);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2665);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2666);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2667);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2668);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2669);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2670);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2671);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2672);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2673);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2674);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2675);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2676);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2677);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2678);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2679);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2680);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2681);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2682);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2683);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2684);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2685);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2686);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2687);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2688);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2689);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2690);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2691);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2692);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2693);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2694);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2695);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2696);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2697);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2698);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2699);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2700);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2701);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2702);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2703);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2704);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2705);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2706);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2707);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2708);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_digit(var& ch) {
        pushNum(ch.value());
        call(2709);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_newline(var& ch) {
        pushNum(ch.value());
        call(2710);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_alpha(var& ch) {
        pushNum(ch.value());
        call(2711);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_alpha_num(var& ch) {
        pushNum(ch.value());
        call(2712);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var to_lower(var& ch) {
        pushNum(ch.value());
        call(2713);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var to_upper(var& ch) {
        pushNum(ch.value());
        call(2714);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void _char12(object $instance) {
        pushObj($instance);
        call(2715);
    }

scope_end()

scope_begin(std, byte)

    void byte(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2716);
    }

    void byte2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2717);
    }

    void byte3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2718);
    }

    void byte4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2719);
    }

    void byte5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2720);
    }

    void byte6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2721);
    }

    void byte7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2722);
    }

    void byte8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2723);
    }

    void byte9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2724);
    }

    void byte10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2725);
    }

    void byte11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2726);
    }

    _uint8 op_$not(object $instance) {
        pushObj($instance);
        call(2727);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2728);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2729);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2730);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2731);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2732);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2733);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2734);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2735);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2736);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2737);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2738);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2739);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2740);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2741);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2742);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2743);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2744);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2745);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2746);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2747);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2748);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2749);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2750);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2751);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2752);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2753);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2754);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2755);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2756);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2757);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2758);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2759);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2760);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2761);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2762);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2763);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2764);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2765);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2766);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2767);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2768);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2769);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2770);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2771);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2772);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2773);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2774);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2775);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2776);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2777);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2778);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2779);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2780);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2781);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2782);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2783);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2784);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2785);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2786);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2787);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2788);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2789);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2790);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2791);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2792);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2793);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2794);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2795);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2796);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2797);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2798);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2799);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2800);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2801);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2802);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2803);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2804);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2805);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2806);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2807);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2808);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_digit(var& ch) {
        pushNum(ch.value());
        call(2809);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_newline(var& ch) {
        pushNum(ch.value());
        call(2810);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_alpha(var& ch) {
        pushNum(ch.value());
        call(2811);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var is_alpha_num(var& ch) {
        pushNum(ch.value());
        call(2812);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var to_lower(var& ch) {
        pushNum(ch.value());
        call(2813);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var to_upper(var& ch) {
        pushNum(ch.value());
        call(2814);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void byte12(object $instance) {
        pushObj($instance);
        call(2815);
    }

scope_end()

scope_begin(std, _bool)

    void _bool(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2816);
    }

    void _bool2(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2817);
    }

    void _bool3(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2818);
    }

    void _bool4(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2819);
    }

    void _bool5(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2820);
    }

    void _bool6(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2821);
    }

    void _bool7(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2822);
    }

    void _bool8(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2823);
    }

    void _bool9(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2824);
    }

    void _bool10(object $instance, object i) {
        pushObj($instance);
        pushObj(i);
        call(2825);
    }

    void _bool11(object $instance, var& i) {
        pushObj($instance);
        pushNum(i.value());
        call(2826);
    }

    _int8 op_$not(object $instance) {
        pushObj($instance);
        call(2827);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_plus(object $instance) {
        pushObj($instance);
        call(2828);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$plus_plus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2829);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_minus(object $instance) {
        pushObj($instance);
        call(2830);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$minus_minus2(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2831);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2832);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2833);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2834);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2835);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2836);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2837);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2838);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2839);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2840);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2841);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2842);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2843);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2844);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2845);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2846);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2847);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2848);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2849);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2850);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2851);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2852);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2853);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2854);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2855);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2856);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2857);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2858);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2859);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2860);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2861);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2862);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2863);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2864);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2865);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2866);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2867);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2868);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2869);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2870);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2871);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2872);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2873);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2874);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2875);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2876);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2877);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2878);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2879);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2880);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2881);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2882);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2883);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2884);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2885);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2886);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2887);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2888);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2889);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2890);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2891);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2892);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2893);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2894);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2895);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2896);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2897);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2898);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2899);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2900);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2901);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2902);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2903);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2904);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2905);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2906);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(2907);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or_equal11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2908);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var check_value(object $instance, var& new_val) {
        pushObj($instance);
        pushNum(new_val.value());
        call(2909);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void _bool12(object $instance) {
        pushObj($instance);
        call(2910);
    }

scope_end()

scope_begin(std, printable)

    object to_string(object $instance) {
        pushObj($instance);
        call(2911);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void printable(object $instance) {
        pushObj($instance);
        call(2912);
    }

scope_end()

scope_begin(std_reflect)

    void __srt_global(object $instance) {
        pushObj($instance);
        call(7);
    }

scope_end()

scope_begin(std_reflect, _class_)

    void _class_(object $instance) {
        pushObj($instance);
        call(2913);
    }

scope_end()

scope_begin(std_reflect, data_entity)

    void data_entity(object $instance) {
        pushObj($instance);
        call(2914);
    }

scope_end()

scope_begin(std_reflect, data_property)

    void data_property(object $instance) {
        pushObj($instance);
        call(2915);
    }

scope_end()

scope_begin(std_reflect, field)

    void field(object $instance) {
        pushObj($instance);
        call(2916);
    }

scope_end()

scope_begin(std_reflect, function)

    void function(object $instance) {
        pushObj($instance);
        call(2917);
    }

scope_end()

scope_begin(std_reflect, reflect)

    void reflect(object $instance, object ref_obj) {
        pushObj($instance);
        pushObj(ref_obj);
        call(2918);
    }

    void reflect2(object $instance) {
        pushObj($instance);
        call(2919);
    }

    void set_reflect_obj(object $instance, object value) {
        pushObj($instance);
        pushObj(value);
        call(3177);
    }

    object get_reflect_obj(object $instance) {
        pushObj($instance);
        call(3178);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, string)

    void string(object $instance, _int8_array& str) {
        pushObj($instance);
        pushObj(str.handle);
        call(2920);
    }

    void string2(object $instance, var& ch) {
        pushObj($instance);
        pushNum(ch.value());
        call(2921);
    }

    void string3(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2922);
    }

    void string4(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2923);
    }

    void string5(object $instance, _int8_array& str, var& offset) {
        pushObj($instance);
        pushObj(str.handle);
        pushNum(offset.value());
        call(2924);
    }

    void string6(object $instance, _int8_array& str, var& start, var& end) {
        pushObj($instance);
        pushObj(str.handle);
        pushNum(start.value());
        pushNum(end.value());
        call(2925);
    }

    object replace(object $instance, var& pos, _int8& ch) {
        pushObj($instance);
        pushNum(pos.value());
        pushNum(ch.value());
        call(2926);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object append(object $instance, var& ch) {
        pushObj($instance);
        pushNum(ch.value());
        call(2927);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var empty(object $instance) {
        pushObj($instance);
        call(2928);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object append2(object $instance, object c) {
        pushObj($instance);
        pushObj(c);
        call(2929);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object append3(object $instance, _int8_array& immstr) {
        pushObj($instance);
        pushObj(immstr.handle);
        call(2930);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object append4(object $instance, _int8_array& immstr, var& len) {
        pushObj($instance);
        pushObj(immstr.handle);
        pushNum(len.value());
        call(2931);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus(object $instance, _int8_array& immstr) {
        pushObj($instance);
        pushObj(immstr.handle);
        call(2932);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_lower(object $instance) {
        pushObj($instance);
        call(2933);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus2(object $instance, object obj) {
        pushObj($instance);
        pushObj(obj);
        call(2934);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus3(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2935);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(2936);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, _int8_array& str) {
        pushObj($instance);
        pushObj(str.handle);
        call(2937);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2938);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, _int8_array& str) {
        pushObj($instance);
        pushObj(str.handle);
        call(2939);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2940);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void op_$equals(object $instance, _int8_array& immstr) {
        pushObj($instance);
        pushObj(immstr.handle);
        call(2941);
    }

    _int8 at(object $instance, var& pos) {
        pushObj($instance);
        pushNum(pos.value());
        call(2942);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$array_at(object $instance, var& pos) {
        pushObj($instance);
        pushNum(pos.value());
        call(2943);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void fmt(var& _fmt) {
        pushNum(_fmt.value());
        call(2944);
    }

    void set_precision(var& prec) {
        pushNum(prec.value());
        call(2945);
    }

    _int8_array get_data(object $instance) {
        pushObj($instance);
        call(2946);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int8_array get_elements(object $instance) {
        pushObj($instance);
        call(2947);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    _int8_array to_array(object $instance) {
        pushObj($instance);
        call(2948);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    var starts_with(object $instance, object prefix) {
        pushObj($instance);
        pushObj(prefix);
        call(2949);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var find(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2950);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var ends_with(object $instance, object postfix) {
        pushObj($instance);
        pushObj(postfix);
        call(2951);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object op_$plus4(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2952);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var size(object $instance) {
        pushObj($instance);
        call(2953);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object parse(var& number) {
        pushNum(number.value());
        call(2954);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2955);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal2(object $instance, _int8& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2956);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal3(object $instance, _uint8& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2957);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal4(object $instance, _int16& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2958);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal5(object $instance, _uint16& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2959);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal6(object $instance, _int32& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2960);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal7(object $instance, _uint32& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2961);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal8(object $instance, _int64& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2962);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal9(object $instance, _uint64& num) {
        pushObj($instance);
        pushNum(num.value());
        call(2963);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal10(object $instance, object obj) {
        pushObj($instance);
        pushObj(obj);
        call(2964);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal11(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2965);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal12(object $instance, _int8_array& str) {
        pushObj($instance);
        pushObj(str.handle);
        call(2966);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object substring(object $instance, var& start_pos, var& end_pos) {
        pushObj($instance);
        pushNum(start_pos.value());
        pushNum(end_pos.value());
        call(2967);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object bounds_error(object $instance, var& pos) {
        pushObj($instance);
        pushNum(pos.value());
        call(2968);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object hash(object $instance) {
        pushObj($instance);
        call(2969);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void string7(object $instance) {
        pushObj($instance);
        call(2970);
    }

    void $03internal_static_init() {
        call(3192);
    }

scope_end()

scope_begin(std, string_builder)

    void set_resize_capacity(object $instance, var& new_capacity) {
        pushObj($instance);
        pushNum(new_capacity.value());
        call(2971);
    }

    void string_builder(object $instance, _int8_array& str) {
        pushObj($instance);
        pushObj(str.handle);
        call(2972);
    }

    void string_builder2(object $instance, var& ch) {
        pushObj($instance);
        pushNum(ch.value());
        call(2973);
    }

    void string_builder3(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2974);
    }

    void string_builder4(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2975);
    }

    void string_builder5(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2976);
    }

    void string_builder6(object $instance, _int8_array& str, var& offset) {
        pushObj($instance);
        pushObj(str.handle);
        pushNum(offset.value());
        call(2977);
    }

    var space_available(object $instance, var& space_required) {
        pushObj($instance);
        pushNum(space_required.value());
        call(2978);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void expand(object $instance, var& space_required) {
        pushObj($instance);
        pushNum(space_required.value());
        call(2979);
    }

    object replace(object $instance, var& pos, _int8& ch) {
        pushObj($instance);
        pushNum(pos.value());
        pushNum(ch.value());
        call(2980);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object append(object $instance, var& ch) {
        pushObj($instance);
        pushNum(ch.value());
        call(2981);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int8_array get_elements(object $instance) {
        pushObj($instance);
        call(2982);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    object append2(object $instance, object c) {
        pushObj($instance);
        pushObj(c);
        call(2983);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object append3(object $instance, _int8_array& immstr) {
        pushObj($instance);
        pushObj(immstr.handle);
        call(2984);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object append4(object $instance, _int8_array& immstr, var& len) {
        pushObj($instance);
        pushObj(immstr.handle);
        pushNum(len.value());
        call(2985);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus(object $instance, _int8_array& immstr) {
        pushObj($instance);
        pushObj(immstr.handle);
        call(2986);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus2(object $instance, object obj) {
        pushObj($instance);
        pushObj(obj);
        call(2987);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(2988);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, _int8_array& str) {
        pushObj($instance);
        pushObj(str.handle);
        call(2989);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2990);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, _int8_array& str) {
        pushObj($instance);
        pushObj(str.handle);
        call(2991);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(2992);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void op_$equals(object $instance, _int8_array& immstr) {
        pushObj($instance);
        pushObj(immstr.handle);
        call(2993);
    }

    _int8 at(object $instance, var& pos) {
        pushObj($instance);
        pushNum(pos.value());
        call(2994);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$array_at(object $instance, var& pos) {
        pushObj($instance);
        pushNum(pos.value());
        call(2995);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void fmt(var& _fmt) {
        pushNum(_fmt.value());
        call(2996);
    }

    void set_precision(var& prec) {
        pushNum(prec.value());
        call(2997);
    }

    _int8_array to_array(object $instance) {
        pushObj($instance);
        call(2998);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    var starts_with(object $instance, object prefix) {
        pushObj($instance);
        pushObj(prefix);
        call(2999);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var find(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(3000);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var find2(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(3001);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var ends_with(object $instance, object postfix) {
        pushObj($instance);
        pushObj(postfix);
        call(3002);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object op_$plus3(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(3003);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var size(object $instance) {
        pushObj($instance);
        call(3004);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object parse(object $instance, var& number) {
        pushObj($instance);
        pushNum(number.value());
        call(3005);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(3006);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal2(object $instance, object obj) {
        pushObj($instance);
        pushObj(obj);
        call(3007);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal3(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(3008);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int8_array get_data(object $instance) {
        pushObj($instance);
        call(3009);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    object op_$plus_equal4(object $instance, object str) {
        pushObj($instance);
        pushObj(str);
        call(3010);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$plus_equal5(object $instance, _int8_array& str) {
        pushObj($instance);
        pushObj(str.handle);
        call(3011);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object substring(object $instance, var& startPos, var& endPos) {
        pushObj($instance);
        pushNum(startPos.value());
        pushNum(endPos.value());
        call(3012);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object bounds_error(object $instance, var& pos) {
        pushObj($instance);
        pushNum(pos.value());
        call(3013);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object hash(object $instance) {
        pushObj($instance);
        call(3014);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void string_builder7(object $instance) {
        pushObj($instance);
        call(3015);
    }

    void $03internal_static_init() {
        call(3193);
    }

scope_end()

scope_begin(std, time)

    var nano_time() {
        call(3016);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var nano_to_micro(var& nano) {
        pushNum(nano.value());
        call(3017);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var nano_to_mills(var& nano) {
        pushNum(nano.value());
        call(3018);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var nano_to_secs(var& nano) {
        pushNum(nano.value());
        call(3019);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var mills_time() {
        call(3020);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void sleep(var& time) {
        pushNum(time.value());
        call(3021);
    }

    void sleep2(object tm_unit, var& time) {
        pushObj(tm_unit);
        pushNum(time.value());
        call(3022);
    }

    void wait(object tm_unit, var& time) {
        pushObj(tm_unit);
        pushNum(time.value());
        call(3023);
    }

    var adjust_time(var& usec) {
        pushNum(usec.value());
        call(3024);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var measure(object tm_unit, var& block) {
        pushObj(tm_unit);
        pushNum(block.value());
        call(3025);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void time(object $instance) {
        pushObj($instance);
        call(3026);
    }

    void $03internal_static_init() {
        call(3194);
    }

scope_end()

scope_begin(std, unique)

    object hash(object $instance) {
        pushObj($instance);
        call(3027);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object guid(object $instance) {
        pushObj($instance);
        call(3028);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void unique(object $instance) {
        pushObj($instance);
        call(3029);
    }

scope_end()

scope_begin(std, loopable$std_io_thread$)

    object get_elements(object $instance) {
        pushObj($instance);
        call(8);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void loopable(object $instance) {
        pushObj($instance);
        call(9);
    }

scope_end()

scope_begin(std, integer$_uint64$)

    void integer(object $instance, var& initial_val) {
        pushObj($instance);
        pushNum(initial_val.value());
        call(10);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(11);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(12);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(13);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(14);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(15);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(16);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(17);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(18);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(19);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(20);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(21);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(22);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(23);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(24);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(25);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(26);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(27);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(28);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(29);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(30);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(31);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(32);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(33);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(34);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(35);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(36);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(37);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(38);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(39);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(40);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(41);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(42);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(43);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(44);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(45);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(46);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(47);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(48);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(49);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(50);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(51);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(52);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(53);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(54);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(55);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(56);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(57);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(58);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(59);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(60);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(61);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(62);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(63);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(64);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(65);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(66);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(67);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(68);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(69);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(70);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(71);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(72);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(73);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(74);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(75);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(76);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(77);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(78);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(79);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(80);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(81);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(82);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(83);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(84);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(85);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(86);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(87);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(88);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(89);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(90);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(91);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(92);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(93);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(94);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(95);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(96);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(97);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(98);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(99);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(100);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(101);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(102);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(103);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(104);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(105);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(106);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(107);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(108);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(109);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(110);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(111);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(112);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(113);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(114);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(115);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(116);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(117);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(118);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(119);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(120);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(121);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(122);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(123);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(124);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(125);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(126);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(127);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(128);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(129);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(130);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(131);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(132);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(133);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(134);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(135);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(136);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(137);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(138);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(139);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(140);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(141);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(142);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(143);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(144);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(145);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(146);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(147);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(148);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(149);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(150);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(151);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(152);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(153);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(154);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(155);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(156);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(157);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(158);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(159);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(160);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(161);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(162);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(163);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(164);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(165);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(166);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(167);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(168);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(169);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(170);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(171);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(172);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(173);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(174);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(175);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(176);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(177);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(178);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(179);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(180);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(181);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(182);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(183);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(184);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(185);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(186);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(187);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint64 op_$equals(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(188);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(189);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _uint64 parse(object s) {
        pushObj(s);
        call(190);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_ulong(object $instance) {
        pushObj($instance);
        call(191);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_long(object $instance) {
        pushObj($instance);
        call(192);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uint(object $instance) {
        pushObj($instance);
        call(193);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_int(object $instance) {
        pushObj($instance);
        call(194);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_ushort(object $instance) {
        pushObj($instance);
        call(195);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_short(object $instance) {
        pushObj($instance);
        call(196);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uchar(object $instance) {
        pushObj($instance);
        call(197);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_char(object $instance) {
        pushObj($instance);
        call(198);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_byte(object $instance) {
        pushObj($instance);
        call(199);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_bool(object $instance) {
        pushObj($instance);
        call(200);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _uint64 get_value(object $instance) {
        pushObj($instance);
        call(201);

        _uint64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void integer2(object $instance) {
        pushObj($instance);
        call(202);
    }

scope_end()

scope_begin(std, integer$_int64$)

    void integer(object $instance, var& initial_val) {
        pushObj($instance);
        pushNum(initial_val.value());
        call(203);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(204);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(205);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(206);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(207);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(208);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(209);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(210);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(211);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(212);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(213);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(214);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(215);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(216);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(217);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(218);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(219);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(220);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(221);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(222);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(223);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(224);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(225);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(226);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(227);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(228);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(229);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(230);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(231);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(232);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(233);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(234);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(235);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(236);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(237);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(238);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(239);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(240);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(241);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(242);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(243);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(244);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(245);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(246);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(247);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(248);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(249);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(250);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(251);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(252);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(253);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(254);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(255);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(256);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(257);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(258);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(259);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(260);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(261);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(262);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(263);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(264);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(265);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(266);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(267);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(268);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(269);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(270);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(271);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(272);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(273);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(274);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(275);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(276);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(277);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(278);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(279);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(280);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(281);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(282);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(283);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(284);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(285);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(286);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(287);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(288);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(289);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(290);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(291);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(292);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(293);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(294);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(295);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(296);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(297);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(298);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(299);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(300);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(301);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(302);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(303);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(304);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(305);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(306);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(307);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(308);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(309);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(310);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(311);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(312);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(313);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(314);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(315);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(316);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(317);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(318);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(319);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(320);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(321);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(322);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(323);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(324);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(325);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(326);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(327);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(328);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(329);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(330);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(331);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(332);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(333);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(334);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(335);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(336);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(337);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(338);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(339);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(340);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(341);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(342);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(343);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(344);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(345);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(346);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(347);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(348);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(349);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(350);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(351);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(352);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(353);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(354);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(355);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(356);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(357);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(358);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(359);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(360);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(361);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(362);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(363);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(364);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(365);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(366);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(367);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(368);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(369);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(370);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(371);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(372);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(373);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(374);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(375);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(376);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(377);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(378);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(379);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(380);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int64 op_$equals(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(381);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(382);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int64 parse(object s) {
        pushObj(s);
        call(383);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_ulong(object $instance) {
        pushObj($instance);
        call(384);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_long(object $instance) {
        pushObj($instance);
        call(385);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uint(object $instance) {
        pushObj($instance);
        call(386);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_int(object $instance) {
        pushObj($instance);
        call(387);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_ushort(object $instance) {
        pushObj($instance);
        call(388);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_short(object $instance) {
        pushObj($instance);
        call(389);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uchar(object $instance) {
        pushObj($instance);
        call(390);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_char(object $instance) {
        pushObj($instance);
        call(391);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_byte(object $instance) {
        pushObj($instance);
        call(392);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_bool(object $instance) {
        pushObj($instance);
        call(393);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int64 get_value(object $instance) {
        pushObj($instance);
        call(394);

        _int64 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void integer2(object $instance) {
        pushObj($instance);
        call(395);
    }

scope_end()

scope_begin(std, integer$_uint32$)

    void integer(object $instance, var& initial_val) {
        pushObj($instance);
        pushNum(initial_val.value());
        call(396);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(397);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(398);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(399);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(400);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(401);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(402);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(403);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(404);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(405);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(406);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(407);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(408);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(409);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(410);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(411);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(412);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(413);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(414);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(415);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(416);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(417);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(418);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(419);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(420);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(421);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(422);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(423);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(424);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(425);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(426);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(427);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(428);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(429);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(430);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(431);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(432);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(433);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(434);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(435);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(436);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(437);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(438);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(439);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(440);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(441);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(442);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(443);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(444);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(445);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(446);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(447);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(448);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(449);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(450);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(451);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(452);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(453);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(454);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(455);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(456);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(457);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(458);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(459);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(460);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(461);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(462);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(463);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(464);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(465);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(466);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(467);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(468);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(469);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(470);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(471);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(472);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(473);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(474);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(475);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(476);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(477);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(478);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(479);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(480);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(481);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(482);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(483);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(484);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(485);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(486);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(487);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(488);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(489);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(490);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(491);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(492);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(493);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(494);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(495);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(496);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(497);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(498);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(499);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(500);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(501);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(502);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(503);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(504);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(505);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(506);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(507);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(508);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(509);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(510);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(511);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(512);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(513);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(514);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(515);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(516);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(517);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(518);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(519);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(520);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(521);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(522);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(523);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(524);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(525);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(526);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(527);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(528);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(529);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(530);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(531);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(532);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(533);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(534);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(535);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(536);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(537);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(538);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(539);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(540);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(541);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(542);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(543);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(544);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(545);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(546);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(547);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(548);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(549);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(550);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(551);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(552);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(553);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(554);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(555);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(556);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(557);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(558);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(559);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(560);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(561);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(562);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(563);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(564);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(565);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(566);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(567);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(568);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(569);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(570);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(571);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(572);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(573);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint32 op_$equals(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(574);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(575);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _uint32 parse(object s) {
        pushObj(s);
        call(576);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_ulong(object $instance) {
        pushObj($instance);
        call(577);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_long(object $instance) {
        pushObj($instance);
        call(578);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uint(object $instance) {
        pushObj($instance);
        call(579);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_int(object $instance) {
        pushObj($instance);
        call(580);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_ushort(object $instance) {
        pushObj($instance);
        call(581);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_short(object $instance) {
        pushObj($instance);
        call(582);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uchar(object $instance) {
        pushObj($instance);
        call(583);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_char(object $instance) {
        pushObj($instance);
        call(584);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_byte(object $instance) {
        pushObj($instance);
        call(585);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_bool(object $instance) {
        pushObj($instance);
        call(586);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _uint32 get_value(object $instance) {
        pushObj($instance);
        call(587);

        _uint32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void integer2(object $instance) {
        pushObj($instance);
        call(588);
    }

scope_end()

scope_begin(std, integer$_int32$)

    void integer(object $instance, var& initial_val) {
        pushObj($instance);
        pushNum(initial_val.value());
        call(589);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(590);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(591);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(592);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(593);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(594);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(595);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(596);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(597);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(598);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(599);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(600);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(601);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(602);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(603);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(604);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(605);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(606);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(607);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(608);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(609);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(610);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(611);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(612);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(613);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(614);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(615);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(616);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(617);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(618);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(619);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(620);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(621);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(622);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(623);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(624);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(625);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(626);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(627);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(628);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(629);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(630);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(631);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(632);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(633);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(634);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(635);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(636);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(637);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(638);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(639);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(640);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(641);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(642);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(643);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(644);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(645);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(646);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(647);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(648);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(649);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(650);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(651);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(652);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(653);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(654);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(655);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(656);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(657);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(658);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(659);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(660);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(661);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(662);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(663);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(664);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(665);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(666);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(667);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(668);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(669);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(670);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(671);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(672);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(673);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(674);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(675);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(676);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(677);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(678);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(679);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(680);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(681);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(682);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(683);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(684);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(685);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(686);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(687);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(688);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(689);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(690);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(691);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(692);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(693);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(694);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(695);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(696);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(697);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(698);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(699);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(700);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(701);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(702);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(703);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(704);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(705);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(706);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(707);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(708);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(709);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(710);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(711);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(712);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(713);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(714);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(715);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(716);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(717);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(718);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(719);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(720);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(721);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(722);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(723);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(724);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(725);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(726);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(727);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(728);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(729);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(730);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(731);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(732);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(733);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(734);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(735);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(736);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(737);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(738);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(739);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(740);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(741);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(742);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(743);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(744);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(745);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(746);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(747);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(748);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(749);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(750);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(751);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(752);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(753);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(754);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(755);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(756);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(757);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(758);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(759);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(760);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(761);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(762);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(763);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(764);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(765);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(766);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int32 op_$equals(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(767);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(768);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int32 parse(object s) {
        pushObj(s);
        call(769);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_ulong(object $instance) {
        pushObj($instance);
        call(770);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_long(object $instance) {
        pushObj($instance);
        call(771);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uint(object $instance) {
        pushObj($instance);
        call(772);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_int(object $instance) {
        pushObj($instance);
        call(773);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_ushort(object $instance) {
        pushObj($instance);
        call(774);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_short(object $instance) {
        pushObj($instance);
        call(775);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uchar(object $instance) {
        pushObj($instance);
        call(776);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_char(object $instance) {
        pushObj($instance);
        call(777);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_byte(object $instance) {
        pushObj($instance);
        call(778);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_bool(object $instance) {
        pushObj($instance);
        call(779);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int32 get_value(object $instance) {
        pushObj($instance);
        call(780);

        _int32 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void integer2(object $instance) {
        pushObj($instance);
        call(781);
    }

scope_end()

scope_begin(std, integer$_uint16$)

    void integer(object $instance, var& initial_val) {
        pushObj($instance);
        pushNum(initial_val.value());
        call(782);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(783);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(784);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(785);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(786);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(787);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(788);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(789);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(790);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(791);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(792);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(793);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(794);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(795);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(796);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(797);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(798);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(799);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(800);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(801);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(802);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(803);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(804);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(805);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(806);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(807);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(808);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(809);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(810);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(811);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(812);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(813);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(814);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(815);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(816);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(817);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(818);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(819);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(820);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(821);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(822);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(823);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(824);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(825);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(826);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(827);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(828);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(829);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(830);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(831);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(832);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(833);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(834);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(835);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(836);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(837);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(838);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(839);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(840);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(841);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(842);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(843);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(844);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(845);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(846);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(847);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(848);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(849);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(850);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(851);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(852);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(853);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(854);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(855);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(856);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(857);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(858);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(859);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(860);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(861);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(862);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(863);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(864);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(865);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(866);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(867);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(868);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(869);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(870);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(871);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(872);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(873);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(874);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(875);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(876);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(877);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(878);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(879);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(880);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(881);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(882);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(883);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(884);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(885);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(886);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(887);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(888);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(889);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(890);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(891);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(892);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(893);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(894);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(895);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(896);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(897);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(898);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(899);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(900);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(901);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(902);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(903);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(904);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(905);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(906);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(907);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(908);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(909);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(910);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(911);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(912);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(913);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(914);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(915);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(916);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(917);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(918);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(919);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(920);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(921);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(922);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(923);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(924);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(925);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(926);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(927);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(928);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(929);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(930);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(931);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(932);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(933);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(934);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(935);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(936);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(937);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(938);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(939);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(940);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(941);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(942);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(943);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(944);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(945);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(946);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(947);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(948);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(949);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(950);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(951);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(952);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(953);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(954);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(955);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(956);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(957);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(958);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(959);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint16 op_$equals(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(960);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(961);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _uint16 parse(object s) {
        pushObj(s);
        call(962);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_ulong(object $instance) {
        pushObj($instance);
        call(963);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_long(object $instance) {
        pushObj($instance);
        call(964);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uint(object $instance) {
        pushObj($instance);
        call(965);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_int(object $instance) {
        pushObj($instance);
        call(966);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_ushort(object $instance) {
        pushObj($instance);
        call(967);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_short(object $instance) {
        pushObj($instance);
        call(968);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uchar(object $instance) {
        pushObj($instance);
        call(969);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_char(object $instance) {
        pushObj($instance);
        call(970);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_byte(object $instance) {
        pushObj($instance);
        call(971);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_bool(object $instance) {
        pushObj($instance);
        call(972);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _uint16 get_value(object $instance) {
        pushObj($instance);
        call(973);

        _uint16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void integer2(object $instance) {
        pushObj($instance);
        call(974);
    }

scope_end()

scope_begin(std, integer$_int16$)

    void integer(object $instance, var& initial_val) {
        pushObj($instance);
        pushNum(initial_val.value());
        call(975);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(976);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(977);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(978);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(979);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(980);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(981);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(982);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(983);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(984);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(985);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(986);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(987);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(988);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(989);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(990);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(991);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(992);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(993);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(994);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(995);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(996);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(997);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(998);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(999);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1000);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1001);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1002);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1003);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1004);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1005);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1006);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1007);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1008);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1009);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1010);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1011);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1012);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1013);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1014);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1015);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1016);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1017);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1018);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1019);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1020);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1021);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1022);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1023);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1024);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1025);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1026);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1027);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1028);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1029);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1030);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1031);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1032);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1033);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1034);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1035);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1036);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1037);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1038);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1039);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1040);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1041);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1042);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1043);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1044);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1045);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1046);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1047);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1048);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1049);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1050);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1051);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1052);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1053);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1054);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1055);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1056);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1057);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1058);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1059);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1060);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1061);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1062);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1063);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1064);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1065);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1066);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1067);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1068);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1069);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1070);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1071);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1072);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1073);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1074);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1075);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1076);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1077);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1078);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1079);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1080);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1081);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1082);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1083);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1084);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1085);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1086);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1087);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1088);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1089);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1090);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1091);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1092);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1093);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1094);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1095);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1096);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1097);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1098);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1099);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1100);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1101);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1102);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1103);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1104);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1105);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1106);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1107);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1108);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1109);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1110);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1111);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1112);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1113);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1114);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1115);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1116);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1117);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1118);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1119);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1120);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1121);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1122);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1123);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1124);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1125);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1126);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1127);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1128);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1129);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1130);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1131);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1132);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1133);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1134);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1135);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1136);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1137);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1138);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1139);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1140);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1141);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1142);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1143);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1144);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1145);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1146);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1147);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1148);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1149);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1150);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1151);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1152);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int16 op_$equals(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1153);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(1154);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int16 parse(object s) {
        pushObj(s);
        call(1155);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_ulong(object $instance) {
        pushObj($instance);
        call(1156);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_long(object $instance) {
        pushObj($instance);
        call(1157);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uint(object $instance) {
        pushObj($instance);
        call(1158);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_int(object $instance) {
        pushObj($instance);
        call(1159);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_ushort(object $instance) {
        pushObj($instance);
        call(1160);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_short(object $instance) {
        pushObj($instance);
        call(1161);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uchar(object $instance) {
        pushObj($instance);
        call(1162);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_char(object $instance) {
        pushObj($instance);
        call(1163);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_byte(object $instance) {
        pushObj($instance);
        call(1164);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_bool(object $instance) {
        pushObj($instance);
        call(1165);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int16 get_value(object $instance) {
        pushObj($instance);
        call(1166);

        _int16 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void integer2(object $instance) {
        pushObj($instance);
        call(1167);
    }

scope_end()

scope_begin(std, integer$_uint8$)

    void integer(object $instance, var& initial_val) {
        pushObj($instance);
        pushNum(initial_val.value());
        call(1168);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(1169);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1170);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1171);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1172);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1173);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1174);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1175);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1176);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1177);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1178);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1179);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1180);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1181);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1182);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1183);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1184);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1185);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1186);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1187);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1188);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1189);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1190);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1191);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1192);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1193);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1194);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1195);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1196);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1197);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1198);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1199);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1200);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1201);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1202);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1203);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1204);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1205);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1206);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1207);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1208);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1209);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1210);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1211);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1212);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1213);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1214);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1215);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1216);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1217);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1218);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1219);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1220);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1221);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1222);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1223);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1224);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1225);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1226);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1227);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1228);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1229);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1230);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1231);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1232);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1233);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1234);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1235);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1236);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1237);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1238);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1239);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1240);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1241);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1242);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1243);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1244);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1245);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1246);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1247);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1248);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1249);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1250);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1251);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1252);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1253);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1254);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1255);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1256);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1257);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1258);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1259);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1260);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1261);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1262);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1263);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1264);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1265);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1266);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1267);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1268);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1269);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1270);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1271);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1272);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1273);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1274);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1275);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1276);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1277);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1278);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1279);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1280);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1281);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1282);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1283);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1284);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1285);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1286);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1287);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1288);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1289);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1290);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1291);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1292);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1293);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1294);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1295);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1296);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1297);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1298);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1299);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1300);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1301);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1302);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1303);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1304);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1305);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1306);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1307);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1308);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1309);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1310);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1311);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1312);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1313);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1314);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1315);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1316);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1317);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1318);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1319);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1320);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1321);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1322);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1323);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1324);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1325);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1326);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1327);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1328);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1329);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1330);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1331);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1332);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1333);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1334);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1335);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1336);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1337);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1338);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1339);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1340);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1341);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1342);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1343);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1344);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1345);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _uint8 op_$equals(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1346);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(1347);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _uint8 parse(object s) {
        pushObj(s);
        call(1348);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_ulong(object $instance) {
        pushObj($instance);
        call(1349);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_long(object $instance) {
        pushObj($instance);
        call(1350);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uint(object $instance) {
        pushObj($instance);
        call(1351);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_int(object $instance) {
        pushObj($instance);
        call(1352);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_ushort(object $instance) {
        pushObj($instance);
        call(1353);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_short(object $instance) {
        pushObj($instance);
        call(1354);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uchar(object $instance) {
        pushObj($instance);
        call(1355);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_char(object $instance) {
        pushObj($instance);
        call(1356);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_byte(object $instance) {
        pushObj($instance);
        call(1357);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_bool(object $instance) {
        pushObj($instance);
        call(1358);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _uint8 get_value(object $instance) {
        pushObj($instance);
        call(1359);

        _uint8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void integer2(object $instance) {
        pushObj($instance);
        call(1360);
    }

scope_end()

scope_begin(std, integer$_int8$)

    void integer(object $instance, var& initial_val) {
        pushObj($instance);
        pushNum(initial_val.value());
        call(1361);
    }

    object hash(object $instance) {
        pushObj($instance);
        call(1362);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var op_$not_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1363);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1364);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1365);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1366);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1367);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1368);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1369);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1370);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1371);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1372);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1373);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1374);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1375);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1376);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1377);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1378);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1379);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1380);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1381);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1382);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1383);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1384);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1385);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1386);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1387);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1388);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1389);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1390);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1391);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1392);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$plus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1393);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1394);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1395);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1396);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1397);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1398);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1399);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1400);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1401);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1402);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1403);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$minus11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1404);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1405);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1406);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1407);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1408);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1409);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1410);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1411);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1412);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1413);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1414);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mult11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1415);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1416);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1417);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1418);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1419);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1420);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1421);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1422);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1423);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1424);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1425);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$div11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1426);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1427);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1428);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1429);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1430);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1431);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1432);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1433);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1434);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1435);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1436);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$mod11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1437);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1438);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1439);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1440);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1441);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1442);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1443);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1444);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1445);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1446);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1447);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1448);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1449);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1450);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1451);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1452);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1453);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1454);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1455);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1456);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1457);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1458);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$greater_than11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1459);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1460);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1461);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1462);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1463);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1464);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1465);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1466);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1467);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1468);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1469);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$less_or_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1470);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1471);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1472);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1473);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1474);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1475);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1476);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1477);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1478);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1479);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1480);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$pow11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1481);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1482);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1483);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1484);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1485);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1486);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1487);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1488);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1489);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1490);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1491);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$left_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1492);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1493);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1494);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1495);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1496);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1497);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1498);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1499);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1500);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1501);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1502);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$right_shift11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1503);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1504);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1505);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1506);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1507);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1508);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1509);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1510);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1511);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1512);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1513);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$xor11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1514);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1515);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1516);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1517);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1518);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1519);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1520);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1521);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1522);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1523);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1524);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$and11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1525);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1526);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or2(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1527);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or3(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1528);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or4(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1529);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or5(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1530);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or6(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1531);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or7(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1532);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or8(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1533);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or9(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1534);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or10(object $instance, object num) {
        pushObj($instance);
        pushObj(num);
        call(1535);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$or11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1536);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$not_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1537);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var op_$equals_equals11(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1538);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    _int8 op_$equals(object $instance, var& num) {
        pushObj($instance);
        pushNum(num.value());
        call(1539);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(1540);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int8 parse(object s) {
        pushObj(s);
        call(1541);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_ulong(object $instance) {
        pushObj($instance);
        call(1542);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_long(object $instance) {
        pushObj($instance);
        call(1543);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uint(object $instance) {
        pushObj($instance);
        call(1544);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_int(object $instance) {
        pushObj($instance);
        call(1545);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_ushort(object $instance) {
        pushObj($instance);
        call(1546);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_short(object $instance) {
        pushObj($instance);
        call(1547);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_uchar(object $instance) {
        pushObj($instance);
        call(1548);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_char(object $instance) {
        pushObj($instance);
        call(1549);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_byte(object $instance) {
        pushObj($instance);
        call(1550);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object to_bool(object $instance) {
        pushObj($instance);
        call(1551);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    _int8 get_value(object $instance) {
        pushObj($instance);
        call(1552);

        _int8 $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void integer2(object $instance) {
        pushObj($instance);
        call(1553);
    }

scope_end()

scope_begin(std, loopable$_int8$)

    _int8_array get_elements(object $instance) {
        pushObj($instance);
        call(1554);

        object $tmpObj = getSpObjAt(0);
        _int8_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    void loopable(object $instance) {
        pushObj($instance);
        call(1555);
    }

scope_end()

scope_begin(std, list$std_string$)

    void list(object $instance) {
        pushObj($instance);
        call(3032);
    }

    void list2(object $instance, var& starting_capacity) {
        pushObj($instance);
        pushNum(starting_capacity.value());
        call(3033);
    }

    void list3(object $instance, object initial_data) {
        pushObj($instance);
        pushObj(initial_data);
        call(3034);
    }

    var empty(object $instance) {
        pushObj($instance);
        call(3035);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void set_compact(object $instance, var& compact_array) {
        pushObj($instance);
        pushNum(compact_array.value());
        call(3036);
    }

    object get_elements(object $instance) {
        pushObj($instance);
        call(3037);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$array_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3038);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3039);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object last(object $instance) {
        pushObj($instance);
        call(3040);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object put(object $instance, var& index, object element) {
        pushObj($instance);
        pushNum(index.value());
        pushObj(element);
        call(3041);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void expand(object $instance) {
        pushObj($instance);
        call(3042);
    }

    void clear(object $instance) {
        pushObj($instance);
        call(3043);
    }

    void add_all(object $instance, object lst) {
        pushObj($instance);
        pushObj(lst);
        call(3044);
    }

    void add_all2(object $instance, object lst) {
        pushObj($instance);
        pushObj(lst);
        call(3045);
    }

    void add(object $instance, object element) {
        pushObj($instance);
        pushObj(element);
        call(3046);
    }

    var indexof(object $instance, object element) {
        pushObj($instance);
        pushObj(element);
        call(3047);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void pop_back(object $instance) {
        pushObj($instance);
        call(3048);
    }

    var size(object $instance) {
        pushObj($instance);
        call(3049);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void insert(object $instance, var& index, object element) {
        pushObj($instance);
        pushNum(index.value());
        pushObj(element);
        call(3050);
    }

    void remove(object $instance, object val) {
        pushObj($instance);
        pushObj(val);
        call(3051);
    }

    void remove2(object $instance, object val, var& compare_fun) {
        pushObj($instance);
        pushObj(val);
        pushNum(compare_fun.value());
        call(3052);
    }

    void remove_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3053);
    }

    var indexof2(object $instance, object comparer, var& find_func) {
        pushObj($instance);
        pushObj(comparer);
        pushNum(find_func.value());
        call(3054);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(3055);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object out_of_bounds_msg(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3056);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, loopable$std_string$)

    object get_elements(object $instance) {
        pushObj($instance);
        call(3030);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void loopable(object $instance) {
        pushObj($instance);
        call(3031);
    }

scope_end()

scope_begin(std, hashtable$std_io_thread_0_std_io_task_job_controller$)

    void hashtable(object $instance, var& initialCapacity) {
        pushObj($instance);
        pushNum(initialCapacity.value());
        call(3057);
    }

    void hashtable2(object $instance) {
        pushObj($instance);
        call(3058);
    }

    void set_threshold(object $instance, var& threshold) {
        pushObj($instance);
        pushNum(threshold.value());
        call(3059);
    }

    var hash(object $instance, object key) {
        pushObj($instance);
        pushObj(key);
        call(3060);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void resize(object $instance) {
        pushObj($instance);
        call(3061);
    }

    object at(object $instance, object key) {
        pushObj($instance);
        pushObj(key);
        call(3062);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var put(object $instance, object key, object value) {
        pushObj($instance);
        pushObj(key);
        pushObj(value);
        call(3063);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var remove(object $instance, object key) {
        pushObj($instance);
        pushObj(key);
        call(3064);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, hashmap$std_int_0_std_io_thread$)

    void hashmap(object $instance, var& initialCapacity) {
        pushObj($instance);
        pushNum(initialCapacity.value());
        call(3065);
    }

    void hashmap2(object $instance) {
        pushObj($instance);
        call(3066);
    }

    void set_threshold(object $instance, var& threshold) {
        pushObj($instance);
        pushNum(threshold.value());
        call(3067);
    }

    var hash(object $instance, object key) {
        pushObj($instance);
        pushObj(key);
        call(3068);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void resize(object $instance) {
        pushObj($instance);
        call(3069);
    }

    object at(object $instance, object key) {
        pushObj($instance);
        pushObj(key);
        call(3070);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    var put(object $instance, object key, object value) {
        pushObj($instance);
        pushObj(key);
        pushObj(value);
        call(3071);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var remove(object $instance, object key) {
        pushObj($instance);
        pushObj(key);
        call(3072);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, list$std_io_thread$)

    void list(object $instance) {
        pushObj($instance);
        call(3074);
    }

    void list2(object $instance, var& starting_capacity) {
        pushObj($instance);
        pushNum(starting_capacity.value());
        call(3075);
    }

    void list3(object $instance, object initial_data) {
        pushObj($instance);
        pushObj(initial_data);
        call(3076);
    }

    var empty(object $instance) {
        pushObj($instance);
        call(3077);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void set_compact(object $instance, var& compact_array) {
        pushObj($instance);
        pushNum(compact_array.value());
        call(3078);
    }

    object get_elements(object $instance) {
        pushObj($instance);
        call(3079);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$array_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3080);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3081);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object last(object $instance) {
        pushObj($instance);
        call(3082);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object put(object $instance, var& index, object element) {
        pushObj($instance);
        pushNum(index.value());
        pushObj(element);
        call(3083);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void expand(object $instance) {
        pushObj($instance);
        call(3084);
    }

    void clear(object $instance) {
        pushObj($instance);
        call(3085);
    }

    void add_all(object $instance, object lst) {
        pushObj($instance);
        pushObj(lst);
        call(3086);
    }

    void add_all2(object $instance, object lst) {
        pushObj($instance);
        pushObj(lst);
        call(3087);
    }

    void add(object $instance, object element) {
        pushObj($instance);
        pushObj(element);
        call(3088);
    }

    var indexof(object $instance, object element) {
        pushObj($instance);
        pushObj(element);
        call(3089);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void pop_back(object $instance) {
        pushObj($instance);
        call(3090);
    }

    var size(object $instance) {
        pushObj($instance);
        call(3091);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void insert(object $instance, var& index, object element) {
        pushObj($instance);
        pushNum(index.value());
        pushObj(element);
        call(3092);
    }

    void remove(object $instance, object val) {
        pushObj($instance);
        pushObj(val);
        call(3093);
    }

    void remove2(object $instance, object val, var& compare_fun) {
        pushObj($instance);
        pushObj(val);
        pushNum(compare_fun.value());
        call(3094);
    }

    void remove_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3095);
    }

    var indexof2(object $instance, object comparer, var& find_func) {
        pushObj($instance);
        pushObj(comparer);
        pushNum(find_func.value());
        call(3096);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(3097);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object out_of_bounds_msg(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3098);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, synced_list$std_io_task_job$)

    void synced_list(object $instance) {
        pushObj($instance);
        call(3101);
    }

    void synced_list2(object $instance, var& starting_capacity) {
        pushObj($instance);
        pushNum(starting_capacity.value());
        call(3102);
    }

    void set_compact(object $instance, var& compact_array) {
        pushObj($instance);
        pushNum(compact_array.value());
        call(3103);
    }

    object get_elements(object $instance) {
        pushObj($instance);
        call(3104);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$array_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3105);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3106);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object last(object $instance) {
        pushObj($instance);
        call(3107);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object put(object $instance, var& index, object element) {
        pushObj($instance);
        pushNum(index.value());
        pushObj(element);
        call(3108);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void expand(object $instance) {
        pushObj($instance);
        call(3109);
    }

    void clear(object $instance) {
        pushObj($instance);
        call(3110);
    }

    void add_all(object $instance, object lst) {
        pushObj($instance);
        pushObj(lst);
        call(3136);
    }

    void add(object $instance, object element) {
        pushObj($instance);
        pushObj(element);
        call(3137);
    }

    var indexof(object $instance, object element) {
        pushObj($instance);
        pushObj(element);
        call(3138);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void pop_back(object $instance) {
        pushObj($instance);
        call(3139);
    }

    var size(object $instance) {
        pushObj($instance);
        call(3140);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var empty(object $instance) {
        pushObj($instance);
        call(3141);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void insert(object $instance, var& index, object element) {
        pushObj($instance);
        pushNum(index.value());
        pushObj(element);
        call(3142);
    }

    void remove(object $instance, object val) {
        pushObj($instance);
        pushObj(val);
        call(3143);
    }

    void remove2(object $instance, object val, var& compare_fun) {
        pushObj($instance);
        pushObj(val);
        pushNum(compare_fun.value());
        call(3144);
    }

    void remove_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3145);
    }

    var indexof2(object $instance, object comparer, var& find_func) {
        pushObj($instance);
        pushObj(comparer);
        pushNum(find_func.value());
        call(3146);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(3147);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object out_of_bounds_msg(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3148);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, loopable$std_io_task_job$)

    object get_elements(object $instance) {
        pushObj($instance);
        call(3099);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void loopable(object $instance) {
        pushObj($instance);
        call(3100);
    }

scope_end()

scope_begin(std, list$std_io_task_job$)

    void list(object $instance) {
        pushObj($instance);
        call(3111);
    }

    void list2(object $instance, var& starting_capacity) {
        pushObj($instance);
        pushNum(starting_capacity.value());
        call(3112);
    }

    void list3(object $instance, object initial_data) {
        pushObj($instance);
        pushObj(initial_data);
        call(3113);
    }

    var empty(object $instance) {
        pushObj($instance);
        call(3114);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void set_compact(object $instance, var& compact_array) {
        pushObj($instance);
        pushNum(compact_array.value());
        call(3115);
    }

    object get_elements(object $instance) {
        pushObj($instance);
        call(3116);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object op_$array_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3117);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3118);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object last(object $instance) {
        pushObj($instance);
        call(3119);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object put(object $instance, var& index, object element) {
        pushObj($instance);
        pushNum(index.value());
        pushObj(element);
        call(3120);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void expand(object $instance) {
        pushObj($instance);
        call(3121);
    }

    void clear(object $instance) {
        pushObj($instance);
        call(3122);
    }

    void add_all(object $instance, object lst) {
        pushObj($instance);
        pushObj(lst);
        call(3123);
    }

    void add_all2(object $instance, object lst) {
        pushObj($instance);
        pushObj(lst);
        call(3124);
    }

    void add(object $instance, object element) {
        pushObj($instance);
        pushObj(element);
        call(3125);
    }

    var indexof(object $instance, object element) {
        pushObj($instance);
        pushObj(element);
        call(3126);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void pop_back(object $instance) {
        pushObj($instance);
        call(3127);
    }

    var size(object $instance) {
        pushObj($instance);
        call(3128);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void insert(object $instance, var& index, object element) {
        pushObj($instance);
        pushNum(index.value());
        pushObj(element);
        call(3129);
    }

    void remove(object $instance, object val) {
        pushObj($instance);
        pushObj(val);
        call(3130);
    }

    void remove2(object $instance, object val, var& compare_fun) {
        pushObj($instance);
        pushObj(val);
        pushNum(compare_fun.value());
        call(3131);
    }

    void remove_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3132);
    }

    var indexof2(object $instance, object comparer, var& find_func) {
        pushObj($instance);
        pushObj(comparer);
        pushNum(find_func.value());
        call(3133);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(3134);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object out_of_bounds_msg(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3135);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, list$var$)

    void list(object $instance) {
        pushObj($instance);
        call(3151);
    }

    void list2(object $instance, var& starting_capacity) {
        pushObj($instance);
        pushNum(starting_capacity.value());
        call(3152);
    }

    void list3(object $instance, var_array& initial_data) {
        pushObj($instance);
        pushObj(initial_data.handle);
        call(3153);
    }

    var empty(object $instance) {
        pushObj($instance);
        call(3154);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void set_compact(object $instance, var& compact_array) {
        pushObj($instance);
        pushNum(compact_array.value());
        call(3155);
    }

    var_array get_elements(object $instance) {
        pushObj($instance);
        call(3156);

        object $tmpObj = getSpObjAt(0);
        var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    var op_$array_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3157);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3158);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var last(object $instance) {
        pushObj($instance);
        call(3159);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    var put(object $instance, var& index, var& element) {
        pushObj($instance);
        pushNum(index.value());
        pushNum(element.value());
        call(3160);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void expand(object $instance) {
        pushObj($instance);
        call(3161);
    }

    void clear(object $instance) {
        pushObj($instance);
        call(3162);
    }

    void add_all(object $instance, object lst) {
        pushObj($instance);
        pushObj(lst);
        call(3163);
    }

    void add_all2(object $instance, var_array& lst) {
        pushObj($instance);
        pushObj(lst.handle);
        call(3164);
    }

    void add(object $instance, var& element) {
        pushObj($instance);
        pushNum(element.value());
        call(3165);
    }

    var indexof(object $instance, var& element) {
        pushObj($instance);
        pushNum(element.value());
        call(3166);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void pop_back(object $instance) {
        pushObj($instance);
        call(3167);
    }

    var size(object $instance) {
        pushObj($instance);
        call(3168);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    void insert(object $instance, var& index, var& element) {
        pushObj($instance);
        pushNum(index.value());
        pushNum(element.value());
        call(3169);
    }

    void remove(object $instance, var& val) {
        pushObj($instance);
        pushNum(val.value());
        call(3170);
    }

    void remove2(object $instance, var& val, var& compare_fun) {
        pushObj($instance);
        pushNum(val.value());
        pushNum(compare_fun.value());
        call(3171);
    }

    void remove_at(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3172);
    }

    var indexof2(object $instance, var& comparer, var& find_func) {
        pushObj($instance);
        pushNum(comparer.value());
        pushNum(find_func.value());
        call(3173);

        var $result(getSpNumAt(0));
        decSp(1);
        return $result;
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(3174);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    object out_of_bounds_msg(object $instance, var& index) {
        pushObj($instance);
        pushNum(index.value());
        call(3175);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

scope_end()

scope_begin(std, loopable$var$)

    var_array get_elements(object $instance) {
        pushObj($instance);
        call(3149);

        object $tmpObj = getSpObjAt(0);
        var_array $result(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);		return $result;
    }

    void loopable(object $instance) {
        pushObj($instance);
        call(3150);
    }

scope_end()

scope_begin(std_io_task, entry$std_io_thread_0_std_io_task_job_controller$)

    void entry(object $instance, object key, object value) {
        pushObj($instance);
        pushObj(key);
        pushObj(value);
        call(3179);
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(3180);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void entry2(object $instance) {
        pushObj($instance);
        call(3181);
    }

scope_end()

scope_begin(std_io, entry$std_int_0_std_io_thread$)

    void entry(object $instance, object key, object value) {
        pushObj($instance);
        pushObj(key);
        pushObj(value);
        call(3182);
    }

    object to_string(object $instance) {
        pushObj($instance);
        call(3183);

        object $result = getSpObjAt(0);
        decSp(1);
        return $result;
    }

    void entry2(object $instance) {
        pushObj($instance);
        call(3184);
    }

scope_end()

