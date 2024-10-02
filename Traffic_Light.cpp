
//Using SDL, SDL_image, standard IO, and strings
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
#include <SDL_ttf.h>
#include <fstream>
#include <iostream>
#include <tuple>

#include <mosquitto.h>
#include <thread>
#include <chrono>


#include <stdlib.h>

//Headers
#include "header.h"

#include "LTexture.h"
#include "LTimer.h"
#include "UI.h"
#include "traffic_light_mode.h"

#include <cstdlib>







//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

//Digits 0-9 Texture vector
std::vector<LTexture> gDigits_Texture(10);

//Create UI object
UI gUI;

traffic_light_mode gTraffic_Light_Mode;



//Text to display
std::wstring timer_green_text = L"green : ";

//Text to display
std::wstring timer_yellow_text = L"yellow : ";

//Text to display
std::wstring timer_red_text = L"red : ";

//Text start game
std::wstring start_traffic_light_text = L"Press Enter to start Traffic Ligth";

//relevant for sending and receiving data over MQTT________________________________________
//_________________________________________________________________________________________

//variables for receiving image data
uint8_t* img_data;
int img_len;



//MQTT subscribe
//const char* mqtt_server = "localhost";
//const char* mqtt_server = "192.168.0.102";
const char* mqtt_server = "broker.hivemq.com";

const int mqtt_port = 1883;
const char* mqtt_topic = "picture";
std::vector<uint8_t> receivedImage;

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto* mosq, void* obj, int reason_code)
{
	int rc;
	/* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
	printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if (reason_code != 0) {
		/* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
		mosquitto_disconnect(mosq);
	}

	/* Making subscriptions in the on_connect() callback means that if the
	 * connection drops and is automatically resumed by the client, then the
	 * subscriptions will be recreated when the client reconnects. */
	rc = mosquitto_subscribe(mosq, NULL, "5/cpu/status_request", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

	rc = mosquitto_subscribe(mosq, NULL, "5/sw/traffic_light", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

	rc = mosquitto_subscribe(mosq, NULL, "picture", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

	rc = mosquitto_subscribe(mosq, NULL, "7/sw/parking_spots", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

	rc = mosquitto_subscribe(mosq, NULL, "bb_x", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

	rc = mosquitto_subscribe(mosq, NULL, "bb_y", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

	rc = mosquitto_subscribe(mosq, NULL, "bb_width", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

	rc = mosquitto_subscribe(mosq, NULL, "bb_height", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

}


/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void on_subscribe(struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos)
{
	int i;
	bool have_subscription = false;

	/* In this example we only subscribe to a single topic at once, but a
	 * SUBSCRIBE can contain many topics at once, so this is one way to check
	 * them all. */
	for (i = 0; i < qos_count; i++) {
		printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
		if (granted_qos[i] <= 2) {
			have_subscription = true;
		}
	}
	if (have_subscription == false) {
		/* The broker rejected all of our subscriptions, we know we only sent
		 * the one SUBSCRIBE, so there is no point remaining connected. */
		fprintf(stderr, "Error: All subscriptions rejected.\n");
		mosquitto_disconnect(mosq);
	}
}

bool status_requested = 0;

int bb_x{ 0 }, bb_y{ 0 }, bb_width{ 0 }, bb_height{ 0 };
SDL_Rect BB_pos = { bb_x, bb_y, bb_width, bb_height };

SDL_Rect* bb_pos = &BB_pos;

void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg)
{
	
	if (strcmp(msg->topic, "5/cpu/status_request") == 0) {
		status_requested = 1;
	}
	
	if (strcmp(msg->topic, "5/sw/traffic_light") == 0) {
		//printf("%s %d %s\n", msg->topic, msg->qos, (char*)msg->payload);
		
		std::cout << "ESP32CAM detected traffic light: " << (char*)msg->payload << std::endl;
	}


	//bounding box daten bb_x
	if (strcmp(msg->topic, "bb_x") == 0) {
		//int payload_value = std::stoi((char*)msg->payload);
		std::string str((char*)msg->payload, msg->payloadlen);
		int bb_x = std::stoi(str);
		bb_x = received_image_render_x + (2.5 * bb_x) + 40;
		bb_pos->x = bb_x;
		//std::cout << bb_x << " : this is the received bb_x" << std::endl;
	}

	//bounding box daten bb_y
	if (strcmp(msg->topic, "bb_y") == 0) {
		//int payload_value = std::stoi((char*)msg->payload);
		std::string str((char*)msg->payload, msg->payloadlen);
		int bb_y = std::stoi(str);
		bb_y = received_image_render_y + (2.5 * bb_y);
		bb_pos->y = bb_y;
		//std::cout << bb_y << " : this is the received bb_y" << std::endl;
	}

	//bounding box daten bb_width
	if (strcmp(msg->topic, "bb_width") == 0) {
		//int payload_value = std::stoi((char*)msg->payload);
		std::string str((char*)msg->payload, msg->payloadlen);
		int bb_width = std::stoi(str);
		bb_width = 2.5 * bb_width;
		bb_pos->w = bb_width;
		//std::cout << bb_width << " : this is the received bb_width" << std::endl;
	}

	//bounding box daten bb_height
	if (strcmp(msg->topic, "bb_height") == 0) {
		//int payload_value = std::stoi((char*)msg->payload);
		std::string str((char*)msg->payload, msg->payloadlen);
		int bb_height = std::stoi(str);
		bb_height = 2.5 * bb_height;
		bb_pos->h = bb_height;
		//std::cout << bb_height << " : this is the received bb_height" << std::endl;
	}


		// Check if the topic is "esp32/picture"
	if (strcmp(msg->topic, "picture") == 0) {

		uint8_t* img_data = (uint8_t*)msg->payload;
		int img_len = msg->payloadlen;


		// Define the directory and file name
		std::string directory = "C:/Users/Domin/Desktop/EZS - Projekt/save_images/"; // Specify your directory here
		std::string filename = "received_image.jpg";

		std::string full_path = directory + filename;

		std::ofstream file(full_path, std::ios::out | std::ios::binary);
		if (file.is_open()) {
			file.write(static_cast<char*>(msg->payload), msg->payloadlen);
			file.close();
			//std::cout << "Image saved as " << full_path << std::endl;
		}
		else {
			std::cerr << "Failed to open file: " << full_path << std::endl;
		}

		gUI.display_image(img_data, img_len);
		
		
		if (&bb_pos != NULL) {
			SDL_SetRenderDrawColor(gUI.get_m_renderer(), 0x00, 0xFF, 0x00, 0xFF);
			SDL_RenderDrawRect(gUI.get_m_renderer(), bb_pos);
			SDL_SetRenderDrawColor(gUI.get_m_renderer(), 0x00, 0x00, 0x00, 0xFF);
		}
	}
	
	

	

	//Gruppenprojekt
	if (strcmp(msg->topic, "7/sw/parking_spots") == 0) {
		//int payload_value = std::stoi((char*)msg->payload);
		std::string str((char*)msg->payload, msg->payloadlen);
		int payload_value = std::stoi(str);
		//std::cout << payload_value << " : this is the received payload value" << std::endl;
		if (gTraffic_Light_Mode.get_mode() == auto2) {
			if (payload_value > 10) {
				gUI.render_green_light();
			} else if (payload_value <=5 && payload_value > 0) {
				gUI.render_yellow_light();
			} else if (payload_value <= 0) {
				gUI.render_red_light();
			}
		}
	}
}




int main(int argc, char* args[])
{
	int rc;
	struct mosquitto* mosq;
	

	/* Required before calling other mosquitto functions */
	mosquitto_lib_init();

	/* Create a new client instance.
	 * id = NULL -> ask the broker to generate a client id for us
	 * clean session = true -> the broker should remove old sessions when we connect
	 * obj = NULL -> we aren't passing any of our private data for callbacks
	 */
	mosq = mosquitto_new(NULL, true, NULL);
	if (mosq == NULL) {
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	/* Configure callbacks. This should be done before connecting ideally. */
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	mosquitto_subscribe_callback_set(mosq, on_subscribe);
	


	/* Run the network loop in a blocking call. The only thing we do in this
* example is to print incoming messages, so a blocking call here is fine.
*
* This call will continue forever, carrying automatic reconnections if
* necessary, until the user calls mosquitto_disconnect().
*/
	rc = mosquitto_connect(mosq, mqtt_server, 1883, 60);
	if (rc != MOSQ_ERR_SUCCESS) {
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}

//start mosquitto loop
	mosquitto_loop(mosq,1000,1);


	//Start up SDL and create window
	if (!gUI.init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//for test/experiment purpose
		gWindow = gUI.get_m_window();
		gRenderer = gUI.get_m_renderer();
		//Load media
		if (!gUI.loadMedia())
		{
			printf("Failed to load media!\n");
		}
		else
		{
			//Main loop flag
			bool quit = false;


			//Event handler
			SDL_Event e;


			//Flags for Menu logic
			bool menu = true;
			bool gameloop = false;


			
			// Nachricht veröffentlichen
			const char* message = "Hello, MQTT!";
			int ret_1 = mosquitto_publish(mosq, NULL, "test", strlen(message), message, 0, false);
			if (ret_1 != MOSQ_ERR_SUCCESS) {
				std::cerr << "Failed to publish message: " << mosquitto_strerror(ret_1) << std::endl;
				mosquitto_destroy(mosq);
				mosquitto_lib_cleanup();
				return 1;
			}
			

			std::cout << "Message published successfully." << std::endl;
			
			
			//_____Mosquito_Ende_________________________

			while (!quit) {
				while (menu && !quit) {
					while (!gameloop && !quit) {
						mosquitto_loop(mosq,1000,1);

						gUI.render_start_screen();
						while (SDL_PollEvent(&e) != 0)
						{
							//User requests quit
							if (e.type == SDL_QUIT)
							{
								quit = true;
							}
							else if (e.key.keysym.sym == SDLK_RETURN)
							{	
								//SDL_RenderClear(gRenderer);
								
								SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0x00);
								SDL_RenderClear(gRenderer);
								gameloop = true;
								menu = false;
							}
							else if (e.key.keysym.sym == SDLK_ESCAPE)
							{
								//SDL_RenderClear(gRenderer);

								SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0x00);
								SDL_RenderClear(gRenderer);
								gameloop = false;
								menu = true;
							}


						}
						SDL_RenderPresent(gRenderer);
					}
				}

				//While application is running
				while (gameloop && !quit)
				{
					mosquitto_loop(mosq, 1000, 1);

					//Handle events on queue
					while (SDL_PollEvent(&e) != 0)
					{
						gTraffic_Light_Mode.set_mode(e.type);
						//User requests quit
						if (e.type == SDL_QUIT)
						{
							quit = true;
						}
						else if (e.key.keysym.sym == SDLK_ESCAPE)
						{
							menu = !menu;
							gameloop = false;
						}

						//switch traffic light mode over keyboard off = 0, green = 1, yellow = 3, red = 4, auto1 = 5 (green 20 sec, yellow 2 sec, red 20 sec), auto2 = 6 (waiting for free parking_spots)
						gTraffic_Light_Mode.set_mode(e.key.keysym.sym);

					}

					if (status_requested) {
						// Convert double to std::string
						std::string str_value = std::to_string(gUI.get_time_until_red());

						// Get const char* from std::string
						const char* message = str_value.c_str();

						int ret_2 = mosquitto_publish(mosq, NULL, "5/cpu/time_until_red", strlen(message), message, 0, false);
						if (ret_2 != MOSQ_ERR_SUCCESS) {
							std::cerr << "Failed to publish message: " << mosquitto_strerror(ret_2) << std::endl;
							mosquitto_destroy(mosq);
							mosquitto_lib_cleanup();
							return 1;
						}
						//std::cout << "time_until_red published successfully" << std::endl;
						
						//reset status_requested flag
						status_requested = 0;
					}
					


					// Leere den `receivedImage`-Vektor für das nächste Bild
					receivedImage.clear();


					//render stuff
					gUI.render_traffic_light_mode(gTraffic_Light_Mode.get_mode());
					//Update screen
					SDL_RenderPresent(gRenderer);



				}
			}
		
		
		// Verbindung zum Broker trennen
			mosquitto_disconnect(mosq);

			// Mosquitto-Client zerstören und Bibliothek aufräumen
			mosquitto_destroy(mosq);
			mosquitto_lib_cleanup();

			//Free resources and close SDL
		
			gUI.close();
		}
	}
	return 0;
}
