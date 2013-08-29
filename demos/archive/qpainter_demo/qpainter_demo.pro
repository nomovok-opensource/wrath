TARGET = qpainter_image_demo
TEMPLATE = app
SOURCES += transformation_node.cpp \
    text_item.cpp \
    image_item.cpp \
    draw_item.cpp \
    generic_command_line.cpp \
    test_list.cpp \
    test_widget.cpp \
    test_widgetGL.cpp \
    simple_2d_transformation.cpp \
    main.cpp

HEADERS += transformation_node.hpp \
    text_item.hpp \
    image_item.hpp \
    draw_item.hpp \
    test_list.hpp \
    test_widget.hpp \
    test_widgetGL.hpp \
    simple_2d_transformation.hpp \
    generic_command_line.hpp



QT += opengl
#CONFIG += qt warn_on debug exceptions stl rtti 
