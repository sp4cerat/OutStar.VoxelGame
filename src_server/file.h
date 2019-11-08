char* file_read(char *name, uint *len=NULL)
{
	FILE *file;
	char *buffer;
	unsigned long fileLen;

	//Open file
	file = fopen(name, "rb");
	if (!file)
	{
		error_stop("File %s not found",name);
	}
	
	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);

	if(len)*len=fileLen;

	//Allocate memory
	buffer=(char *)malloc(fileLen+2);
	memset(buffer,0,fileLen+2);
	if (!buffer)
	{
		error_stop("Error allocating %d bytes",fileLen);
	}

	//Read file contents into buffer
	fread(buffer, fileLen, 1, file);
	fclose(file);

	return buffer;
}

void file_write(char *name, char* buffer, uint fileLen)
{
	FILE *file;

	if (!name)
	{
		error_stop("Filename zero pointer");
	}

	//Open file
	file = fopen(name, "wb");

	if (!file)
	{
		error_stop("File %s cannot be created",name);
	}
	
	if (!buffer)
	{
		error_stop("Zero pointer!");
	}

	//Read file contents into buffer
	fwrite(buffer, fileLen, 1, file);
	fclose(file);
}

void file_zwrite(char *filename, char* buffer, uint size)
{
	uint zipped_len=0;
	uint* zipped=(uint*)zip::compress((uchar*)&buffer[0], size,&zipped_len);
	zipped=(uint*)realloc(zipped,zipped_len+8);
	memmove(((uchar*)zipped)+8,zipped,zipped_len);
	zipped[0]=size;
	zipped[1]=zipped_len;
	//loopi(0,zipped_len+8) ((char*)zipped)[i]^=i*11+123;
	file_write(filename, (char*)zipped,zipped_len+8);
	free(zipped);
}

char* file_zread(char *filename, uint *unpackedsize=NULL)
{
	uint  file_len=0;
	uint* zipped = (uint*)file_read(filename,&file_len);	
	//loopi(0,file_len) ((char*)zipped)[i]^=i*11+123;
	uint size			=zipped[0];
	uint zipped_len		=zipped[1];
	uint  unzipped_len=0;
	char* data=(char*)zip::uncompress((uchar*)&zipped[2], zipped_len, &unzipped_len);
	free(zipped);
	data=(char*)realloc(data,size+1);
	data[size]=0;
	if(unpackedsize)*unpackedsize=size;
	return data;
}



