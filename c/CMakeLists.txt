cmake_minimum_required(VERSION 3.1)
project(uAMQP)

include_directories(inc)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(amqp_includes 
	inc/amqpalloc.h
	inc/amqplib.h
	inc/amqpvalue.h
	inc/amqp_definitions.h
	inc/amqp_frame_codec.h
	inc/amqp_types.h
	inc/connection.h
	inc/consolelogger.h
	inc/frame_codec.h
	inc/io.h
	inc/link.h
	inc/list.h
	inc/logger.h
	inc/message.h
	inc/messaging.h
	inc/platform.h
	inc/session.h
	inc/socketio.h
)

add_library(uAMQP
	$<TARGET_OBJECTS:amqplib>
	$<TARGET_OBJECTS:amqpalloc>
	$<TARGET_OBJECTS:amqpvalue>
	$<TARGET_OBJECTS:amqp_definitions>
	$<TARGET_OBJECTS:amqp_frame_codec>
	$<TARGET_OBJECTS:connection>
	$<TARGET_OBJECTS:consolelogger>
	$<TARGET_OBJECTS:frame_codec>
	$<TARGET_OBJECTS:io>
	$<TARGET_OBJECTS:link>
	$<TARGET_OBJECTS:list>
	$<TARGET_OBJECTS:message>
	$<TARGET_OBJECTS:messaging>
	$<TARGET_OBJECTS:platform_win32>
	$<TARGET_OBJECTS:session>
	$<TARGET_OBJECTS:socketio_win32>
	
	${amqp_includes}
	)
	
add_library(amqplib OBJECT
	src/amqplib.c
	${amqp_includes}
	)
	
add_library(socketio_win32 OBJECT
	src/socketio_win32.c
	${amqp_includes}
	)
	
add_library(session OBJECT
	src/session.c
	${amqp_includes}
	)
	
add_library(amqp_frame_codec OBJECT
	src/amqp_frame_codec.c
	${amqp_includes}
	)

add_library(amqpalloc OBJECT
	src/amqpalloc.c
	${amqp_includes}
	)

add_library(amqpvalue OBJECT
	src/amqpvalue.c
	${amqp_includes}
	)
	
add_library(amqp_definitions OBJECT
	src/amqp_definitions.c
	${amqp_includes}
	)
	
add_library(connection OBJECT
	src/connection.c
	${amqp_includes}
	)
	
add_library(consolelogger OBJECT
	src/consolelogger.c
	${amqp_includes}
	)
	
add_library(frame_codec OBJECT
	src/frame_codec.c
	${amqp_includes}
	)
	
add_library(io OBJECT
	src/io.c
	${amqp_includes}
	)

add_library(link OBJECT
	src/link.c
	${amqp_includes}
	)
	
add_library(list OBJECT
	src/list.c
	${amqp_includes}
	)
	
add_library(message OBJECT
	src/message.c
	${amqp_includes}
	)
	
add_library(messaging OBJECT
	src/messaging.c
	${amqp_includes}
	)
	
add_library(platform_win32 OBJECT
	src/platform_win32.c
	${amqp_includes}
	)
	
	
add_subdirectory(samples)
add_subdirectory(testtools)
add_subdirectory(unittests)
