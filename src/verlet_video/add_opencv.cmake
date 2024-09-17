find_package(OpenCV REQUIRED core imgproc videoio imgcodecs highgui)
target_link_libraries(verlet_video PRIVATE ${OpenCV_LIBS})
