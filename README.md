
# The purpose of the project was to use MQTT for transmitting data.

In the following i briefly describe the project

I created in C++ with SDL2 a Graphic of a Traffic Light. 
By pressing on the Keyboard:
  1 = Traffic Light Off
  2 = Green
  3 = Yellow
  4 = Red
  5 = Auto mode

I used this Graphic to create a Object detection model with Edge Impulse
I shot some pictures with the ESP32 Cam and used this picture for the training.
I repeated this several times to achive an acceptable result. My aim was to do the image processing with computer vision right on the ESP32 Cam micro controller.

# Publish
The ESP32 Cam microcontroller publishes the object detection results like bounding box, classification and the picture it self.

# Subscribe
On the other side i subscribe the information on my computer and display the received picture with the bounding boxes and classification within the picture right next to the Traffic Light.

Unfortunately I don't have a screenshot of the Interface with the Traffic Light and the Received data displayed next to it. The micro controller is in Germany an I'm in Australia right now.
  
