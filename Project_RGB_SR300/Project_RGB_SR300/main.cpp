//#include <iostream> // cout - cin 
#include <cstdio> // FILE objects - printf - scanf - fopen
#include <glfw3.h> //Libreria GLFW para display gráfico
//#include "CImg.h" //Libreria C++ image processing
#include <windows.h>
#include <gdiplus.h>


#include <rs.hpp>
#include <FreeImage.h>
#include "SaveImageBmp.h"

//Macro Ancho x largo de la Imagen capturada
#define WIDTH  640
#define HEIGHT 480


//Prototipo
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);


//Global variable
const void* ptr; //Puntero a la frame de la camara
const void* ptrDepth;
bool Shot = true; //Bandera de salida


int main() {


	int width_image = WIDTH;
	int heigh_image = HEIGHT;

	//Crear contexto realsense
	rs::context ctx;

	if (ctx.get_device_count() == 0) {
		printf("Error");
		getchar();
		return -1;
	}
	else {
		printf("OK");
	}
	
	//Crear un dispositivo
	rs::device *SR300Camara = ctx.get_device(0);

	//Habilitar Camara y configurar
	SR300Camara->enable_stream(rs::stream::color, width_image, heigh_image, rs::format::bgr8, 30);
	SR300Camara->enable_stream(rs::stream::depth, width_image, heigh_image, rs::format::z16, 30);

	SR300Camara->start();

	/*-------------------------------------------------------------------
		Libreria para la creación de ventanas y manupulación de eventos
	--------------------------------------------------------------------*/

	GLFWwindow *window;

	/*Inicializar libreria glfw
	------------------------------
	La funcion returna un TRUE si es exitosa.
	*/
	if (!glfwInit())
	{
		printf("Ha ocurrido un error al inicializar la librera");
		return -1;
	}

	//Crea el contexto de OpenGL y establece parametros de la ventana
	window = glfwCreateWindow(1280, 480, "Camara SR300", nullptr, nullptr);

	if (window == NULL) {
		printf("Error al crear el contexto OpenGL");
		return -1;
	}

	//El contexto creado lo asocia a la ventana actual
	glfwMakeContextCurrent(window);


	/*Configurar Evento
	------------------------------
	*/
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	while (!glfwWindowShouldClose(window)) {

		/*Esperar por una frame*/
		SR300Camara->wait_for_frames();

		/*Render aqui*/
		glClear(GL_COLOR_BUFFER_BIT);

		//Si no se aplica, la imagen aparece invertida
		glPixelZoom(1, -1);

		/*Mostrar imagen de la frame de la camara*/
		glRasterPos2f(-1, 1); //posiciona los pixeles
		ptr = SR300Camara->get_frame_data(rs::stream::color);
		glDrawPixels(640, 480, GL_BGR_EXT, GL_UNSIGNED_BYTE, ptr);


		//Mostrar camara de profundidad
		glRasterPos2f(0, 1);
		glPixelTransferf(GL_RED_SCALE, 100);
		ptrDepth = SR300Camara->get_frame_data(rs::stream::depth);
		glDrawPixels(width_image, heigh_image, GL_RED, GL_UNSIGNED_SHORT, ptrDepth);
		glPixelTransferf(GL_RED_SCALE, 1.0f);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		if (!Shot)
		{
			if(!SaveImageToBmp((uint8_t*)ptr, "wound.bmp", width_image, heigh_image)) return -1;


			if(!SaveImageDepth((uint8_t*)ptrDepth, "depth.bmp", width_image, heigh_image)) return -1;

			//Detener transmision
			SR300Camara->stop();

			//Cerrar ventana
			glfwDestroyWindow(window);

			showImageBmp(); // Presionar tecla E para salir del programa


			break;
		}
	
	}

	glfwTerminate();
	return 0;
}

//Evento click
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		Shot = false;
	
	}
}
