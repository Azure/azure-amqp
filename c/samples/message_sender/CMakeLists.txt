add_executable(send
	main.c)
	
#TODO: link to whatever else the platform has (now for windows it is just ws2_32)
target_link_libraries(send uAMQP ws2_32)
