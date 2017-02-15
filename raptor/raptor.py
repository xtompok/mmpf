from ctypes import Structure,uint32,POINTER,c_void_p,cdll

MAX_TRANSFERS=8

class stop_arrivals(Structure):
	_fields_ = [("time",uint_32 * MAX_TRANSFERS),
		("from",uint_32 * MAX_TRANSFERS),
		("fdep",uint_32 * MAX_TRANSFERS),
		("route",uint_32 * MAX_TRANSFERS)]

class mem_data(Structure):
	_fields_ = [("stop_arrivals",POINTER(stop_arrivals))]

raptor = cdll.LoadLibrary("./raptor.so")
load_timetable = raptor.load_timetable
search_con = raptor.search_con

load_timetable.argtypes = [c_char_p]
load_timetable.restype = c_void_p
search_con.argtypes = [c_void_p,POINTER(mem_data),uint_32,uint_32,uint64_t]

