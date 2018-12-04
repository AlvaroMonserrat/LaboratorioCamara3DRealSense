#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <MsXml2.h>
#include "tinyxml.h"

using namespace std;


void CreateFolder(const char * path)
{
	if (!CreateDirectory(path, NULL))
	{
		return;
	}
}


// load the named file and dump its structure to STDOUT
void dump_to_stdout(const char* pFilename)
{
	TiXmlDocument doc(pFilename);
	bool loadOkay = doc.LoadFile();
	if (loadOkay)
	{
		printf("\n%s:\n", pFilename);
	
	}
	else
	{
		printf("Failed to load file \"%s\"\n", pFilename);
	}
}

void write_simple_doc()
{
	TiXmlDocument doc;
	TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", ""); 
	doc.LinkEndChild(decl);

	TiXmlElement *analisis = new TiXmlElement("Analisis");
	doc.LinkEndChild(analisis);

	TiXmlElement *wound = new TiXmlElement("Herida");
	analisis->LinkEndChild(wound);

	/*Area Total*/
	TiXmlElement *at = new TiXmlElement("at"); //햞ea Total
	wound->LinkEndChild(at);

	TiXmlText *text_at = new TiXmlText("30 cm2");  //햞ea Herida total
	at->LinkEndChild(text_at);

	/*Area rojo*/
	TiXmlElement *ar = new TiXmlElement("ar"); //햞ea rojo
	wound->LinkEndChild(ar);

	TiXmlText *text_ar = new TiXmlText("15 cm2");  //햞ea Herida total rojo
	ar->LinkEndChild(text_ar);


	/*Area amarillo*/
	TiXmlElement *aa = new TiXmlElement("aa"); //햞ea amarillo
	wound->LinkEndChild(aa);

	TiXmlText *text_aa = new TiXmlText("10 cm2");  //햞ea Herida total amarillo
	aa->LinkEndChild(text_aa);

	/*Area negro*/
	TiXmlElement *an = new TiXmlElement("an"); //햞ea negro
	wound->LinkEndChild(an);

	TiXmlText *text_an = new TiXmlText("5 cm2");  //햞ea Herida total negro
	an->LinkEndChild(text_an);

	/*Porcentaje rojo*/
	TiXmlElement *pr = new TiXmlElement("pr"); //Porcentaje rojo
	wound->LinkEndChild(pr);

	TiXmlText *text_pr = new TiXmlText("40 %");  //Porcentaje rojo
	pr->LinkEndChild(text_pr);

	/*Porcentaje Amarillo*/
	TiXmlElement *pa = new TiXmlElement("pa"); //Porcentaje Amarillo
	wound->LinkEndChild(pa);

	TiXmlText *text_pa = new TiXmlText("30 %");  //Porcentaje Amarillo
	pa->LinkEndChild(text_pa);

	/*Porcentaje Negro*/
	TiXmlElement *pm = new TiXmlElement("pm"); //Porcentaje Negro (pa)
	wound->LinkEndChild(pm);

	TiXmlText *text_pm = new TiXmlText("30 %");  //Porcentaje Negro
	pm->LinkEndChild(text_pm);

	CreateFolder("C:\\Image_Nurseye\\");

	ofstream myFile;

	doc.SaveFile("C:\\Image_Nurseye\\\wound.xml");

	myFile.close();

}


int main() {


	CreateFolder("C:\\folder_name\\");

	ofstream myFile;

	myFile.open("C:\\folder_name\\testBridge.txt");

	myFile << "Se ha ejecutado el programa.\n";

	myFile.close();

	write_simple_doc();

	cout << "El programse se ejecuto correctamente\n" << endl;
	getchar();

	return 0;
}