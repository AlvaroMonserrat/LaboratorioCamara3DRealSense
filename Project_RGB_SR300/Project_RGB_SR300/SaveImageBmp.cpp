#include "SaveImageBmp.h"



bool SaveImageToBmp(unsigned char *image_camara, char* nameFile, int &width, int &height) {


	// Convert to FreeImage format & save to file
	FIBITMAP* image = FreeImage_ConvertFromRawBits(image_camara, width, height, 3 * width, 24, 0xFF0000, 0x00FF00, 0x0000FF, true);
	bool saved = FreeImage_Save(FIF_BMP, image, nameFile, 0);

	if (saved)
	{
		return true;
	}
	// Free resources
	FreeImage_Unload(image);

	return false;

}

bool SaveImageDepth(unsigned char *image_depth, char* nameFile, int &width, int &height) {

	// Convert to FreeImage format & save to file
	FIBITMAP* image = FreeImage_ConvertFromRawBits(image_depth, width, height,  2*width, 16, 0xFF0000, 0x00FF00, 0x0000FF, true);
	bool saved = FreeImage_Save(FIF_BMP, image, nameFile, 0);

	if (saved)
	{
		return true;
	}
	// Free resources
	FreeImage_Unload(image);

	return false;

}

bool exit_showImage = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
//Mostrar imagen guardada
int showImageBmp() {


	printf("\nPresione Enter para ver la Imagen guardada\n");
	getchar();


	GLFWwindow *windowResult;

	//Crea el contexto de OpenGL y establece parametros de la ventana
	windowResult = glfwCreateWindow(640, 480, "Camara SR300 Result", nullptr, nullptr);

	if (windowResult == NULL) {
		printf("Error al crear el contexto OpenGL");
		return -1;
	}

	//El contexto creado lo asocia a la ventana actual
	glfwMakeContextCurrent(windowResult);


	//Leer Imagen guardada
	unsigned char* imageResult = readBMP("wound.bmp");


	glfwSetKeyCallback(windowResult, key_callback);

	while (!glfwWindowShouldClose(windowResult))
	{
		/*Render aqui*/
		glClear(GL_COLOR_BUFFER_BIT);

		//Si no se aplica, la imagen aparece invertida
		glPixelZoom(1, -1);

		/*Mostrar imagen de la frame de la camara*/
		glRasterPos2f(-1, 1); //posiciona los pixeles
		glDrawPixels(640, 480, GL_BGR_EXT, GL_UNSIGNED_BYTE, imageResult);

		/* Swap front and back buffers */
		glfwSwapBuffers(windowResult);

		/* Poll for and process events */
		glfwPollEvents();

		if (exit_showImage)
		{
			break;
		}

	}

	delete imageResult;

	return 0;

}

//Evento
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		exit_showImage = true;
	}
		
}


//Fucion para leer los datos de la imagen bmp

unsigned char* readBMP(char* filename)
{
	int i;
	FILE* f;

	errno_t ErrorCode = fopen_s(&f, filename, "rb");

	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

											   // extract image height and width from header
	int width = *(int*)&info[18];
	int height = *(int*)&info[22];

	int size = 3 * width * height;
	unsigned char* data = new unsigned char[size]; // allocate 3 bytes per pixel


	fread(data, sizeof(unsigned char), size, f); // read the rest of the data at once
	fclose(f);

	/*
	for (i = 0; i < size; i += 3)
	{
	unsigned char tmp = data[i];
	data[i] = data[i + 2];
	data[i + 2] = tmp;
	}
	*/


	int sizeDinamic = size - 1;
	for (i = 0; i < (size / 2); i += 3, sizeDinamic -= 3)
	{
		unsigned char tempR = data[i];
		unsigned char tempG = data[i + 1];
		unsigned char tempB = data[i + 2];

		data[i] = data[sizeDinamic - 2];
		data[sizeDinamic - 2] = tempR;

		data[i + 1] = data[sizeDinamic - 1];
		data[sizeDinamic - 1] = tempG;

		data[i + 2] = data[sizeDinamic];
		data[sizeDinamic] = tempB;

	}


	return data;
}