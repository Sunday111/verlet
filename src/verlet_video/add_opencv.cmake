find_package(OpenCV REQUIRED core videoio highgui)
target_link_libraries(verlet_video PRIVATE ${OpenCV_LIBS})
