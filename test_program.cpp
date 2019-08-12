// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "benchfiler.h"

int main()
{
	benchfiler::Initialize();

	int x = 0;
	while (x++ < 1000) {
		benchfiler::Begin();
		FILE * file;
		tmpfile_s(&file);
		fclose(file);

		benchfiler::End();
	}

	benchfiler::Report();
}
