#include <iostream>
#include <rs.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <glfw3.h>
#include <math.h>

#define PI 3.14159265

#define WIDTH 640
#define HEIGHT 480

/*Prototype function*/
void PrintInfoDevice(rs::device *device);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);


bool Shot = false; //Bandera de salida

int main()
{
	//Crear Contexto 
	rs::context ctx;
	
	if (!ctx.get_device_count())
	{
		std::cout << "No hay dispositivos conectados\n" << std::endl;
		getchar();
		return -1;
	}

	//Inicializar Camara 0
	rs::device *sr300Camara = ctx.get_device(0);

	if (sr300Camara == NULL)
	{
		std::cout << "Error al inicializar dispositivo" << std::endl;
		getchar();
		return -1;
	}

	/*------------------------------------------------------
		Paramatros obtenidos por el dispositivo
	------------------------------------------------------*/
	PrintInfoDevice(sr300Camara);
	/*------------------------------------------------------
				Fin de parametros
	------------------------------------------------------*/
	float scale = sr300Camara->get_depth_scale(); // escala
	uint16_t centerValueDepthData = 0;
	double distanceZ = 0;
	double numberOfPixel = 0;
	double ancho = 0; // Ancho real final
	double largo = 0; // Largo real final
	double a = 0; // d * tan(anguloHFOV/2)
	double l = 0; // d * tan(anguloVFOV/2)
	int width_color = WIDTH;
	int height_color = HEIGHT;
	const void* ptrNoTypeDataColor;
	const void* ptrNoTypeDataDepth;

	//Configurar transmision RGB
	sr300Camara->enable_stream(rs::stream::color, width_color, height_color, rs::format::bgr8, 30);
	sr300Camara->enable_stream(rs::stream::depth, width_color, height_color, rs::format::z16, 30);

	//Iniciar transmision
	sr300Camara->start();

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

	while (!glfwWindowShouldClose(window))
	{
		//Esperar frame
		sr300Camara->wait_for_frames();

		/*Render aqui*/
		glClear(GL_COLOR_BUFFER_BIT);

		//Si no se aplica, la imagen aparece invertida
		glPixelZoom(1, -1);

		/*Mostrar imagen de la frame de la camara*/
		glRasterPos2f(-1, 1); //posiciona los pixeles
 		ptrNoTypeDataColor = sr300Camara->get_frame_data(rs::stream::color); //Puntero a array of image
		glDrawPixels(width_color, height_color, GL_BGR_EXT, GL_UNSIGNED_BYTE, ptrNoTypeDataColor);

		//Mostrar camara de profundidad
		glRasterPos2f(0, 1);
		glPixelTransferf(GL_RED_SCALE, 100);
		ptrNoTypeDataDepth = sr300Camara->get_frame_data(rs::stream::depth);
		glDrawPixels(width_color, height_color, GL_RED, GL_UNSIGNED_SHORT, ptrNoTypeDataDepth);
		glPixelTransferf(GL_RED_SCALE, 1.0f);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		if (Shot)
		{
			cv::Mat ImageColor(height_color, width_color, CV_8UC3, (uint8_t*)ptrNoTypeDataColor);
			cv::Mat ImageDepth(height_color, width_color, CV_16U, (uint16_t*)ptrNoTypeDataDepth);
			//Guardar imagen
			cv::imwrite("imageColor.png", ImageColor);
			cv::imwrite("ImageDepth.png", ImageDepth);

			//Obtener distancia en cm
			distanceZ = ImageDepth.at<uint16_t>(240, 320) * scale *100;
			
			printf("La distancia centro es: %g\n", distanceZ);

			//Imagen imgHSV para umbralizar
			cv::Mat imgHSV;
			cv::cvtColor(ImageColor, imgHSV, cv::COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

			//Imagen umbralizada
			cv::Mat imgThresholded;
			cv::inRange(imgHSV, cv::Scalar(160, 80, 80), cv::Scalar(230, 255, 255), imgThresholded);

			//cv::imshow("Umbral", imgThresholded);
			
			cv::Mat Puntos;

			//Crear elemento para erosion y dilatacion
			int erosion_size = 3;
			cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));

			//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
			erode(imgThresholded, imgThresholded, element);
			dilate(imgThresholded, imgThresholded, element);

			//Obtener pixeles blancos
			cv::findNonZero(imgThresholded, Puntos);

			//Obtener total de pixeles de la figura encontrada
			numberOfPixel = Puntos.total();

			cv::Rect Min_Rect = cv::boundingRect(Puntos);

			int areaRect = 0;
			areaRect = Min_Rect.area();
			int width = Min_Rect.width;
			int height = Min_Rect.height;
			/*
			//Calculo de pixeles de la figura
			for (int y = 0; y < 480; y++) {
				for (int x = 0; x < 640; x++)
				{
					if (imgThresholded.at<uchar>(y, x) == 255)
					{
						numberOfPixel++;
					}
				}
			}
			*/
			//Calculo del area

			rectangle(ImageColor, Min_Rect.tl(), Min_Rect.br(), cv::Scalar(0, 255, 0), 1);

			cv::imshow("ImageBinary", imgThresholded);
			cv::imshow("Result", ImageColor);

			a = distanceZ * tan(28.5*PI / 180.0);
			l = distanceZ * tan(21.5*PI / 180.0);
			
			ancho = (width * 2.0 * a) / 640.0;
			largo = (height * 2.0 * l) / 480.0;

			printf("Numero de pixeles de la figura: %g\n", numberOfPixel);
			printf("El Ancho de la figura es: %g cm\n", ancho);
			printf("El Largo de la figura es: %g cm\n", largo);
			printf("El area de la caja es: %g cm2\n", ancho*largo);
			printf("El valor de a es: %g cm/px\n", 2*a/640.0);
			printf("El valor de l es: %g cm/px\n", 2*l / 480.0);
			printf("El area de la figura es: %g cm2\n", (2.0*a / 640.0)*(2.0*l / 480.0)*numberOfPixel);
			cv::waitKey(0);

			break;
		
		}
		
	}

	glfwTerminate();

	return 0;
}


void PrintInfoDevice(rs::device *device)
{
	float scale = device->get_depth_scale(); // escala
	std::cout << "factor de scale: " << scale << std::endl;

	double metro = 1.0 / scale; // 1 metro equivalente a datos de profundidad
	std::cout << "Un metro equivale a " << metro << " en datos de profundidad" << std::endl;

	std::string firmware = device->get_firmware_version(); // Firmware
	std::cout << "El firmware del dispositivo es: " << firmware << std::endl;

	std::string nameDevice = device->get_name(); //Nombre del dispositivo
	std::cout << "El nombre del dispositivo es: " << nameDevice << std::endl;

	std::string serial_number = device->get_serial(); //Numero de seria
	std::cout << "El numero de seria del dispositivo es: " << serial_number << std::endl;

	//std::string usb_port = sr300Camara->get_usb_port_id(); //Puerto usb
	//std::cout << "El puerto usb es: " << usb_port << std::endl;

}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		Shot = true;

	}
}


