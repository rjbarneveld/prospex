		string tmpOutFName = ioFolder + "/q_"+to_string(year[index])+"_"+gid+".tif";
		const char* outFName = tmpOutFName.c_str();//"io/erosion_.tif";
		GDALDataset *output;
		GDALDriver *driverGeotiff;
		float *rowBuff = (float*) CPLMalloc(sizeof(float)*xSize);
		driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
					{
					rowBuff[x] = (float)Q_cum[x][y];
					flaccMFD[x][y]=0;
					}
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}		
		GDALClose( (GDALDatasetH) output );

		tmpOutFName = ioFolder + "/qMax_"+to_string(year[index])+"_"+gid+".tif";
		outFName = tmpOutFName.c_str();
		driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
					{
					rowBuff[x] = (int)flaccMAX[x][y];
					flaccMAX[x][y]=0;
					}
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}		
		GDALClose( (GDALDatasetH) output );
		
		tmpOutFName = ioFolder + "/sw_"+to_string(year[index])+"_"+gid+".tif";
		outFName = tmpOutFName.c_str();//"io/erosion_.tif";
		driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
					rowBuff[x] = (float)SW_EHD[x][y]/(SWSat_EHD[x][y]*EHDepth[x][y][1]);
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}		
		GDALClose( (GDALDatasetH) output );
	if(dynamicG==1)
		{
		tmpOutFName = ioFolder + "/latflow_"+to_string(year[index])+"_"+gid+".tif";
		outFName = tmpOutFName.c_str();//"io/erosion_.tif";
		driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
					rowBuff[x] = (float)latflow[x][y];
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}		
		GDALClose( (GDALDatasetH) output );

		tmpOutFName = ioFolder + "/groundwaterLevel_"+to_string(year[index])+"_"+gid+".tif";
		outFName = tmpOutFName.c_str();//"io/erosion_.tif";
		driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData)
					rowBuff[x] = (float)G[x][y][0];
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}		
		GDALClose( (GDALDatasetH) output );
		}
	if(erosion==1)
		{
		string tmpOutFName = ioFolder + "/erosion_"+to_string(year[index])+"_" + to_string(scenario) + "_"+gid+".tif";
		const char* outFName = tmpOutFName.c_str();//"io/erosion_.tif";
		GDALDataset *output;
		GDALDriver *driverGeotiff;
		float *rowBuff = (float*) CPLMalloc(sizeof(float)*xSize);
		driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData)
					rowBuff[x] = (float)eroMFD[x][y];
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}		
		GDALClose( (GDALDatasetH) output );

		tmpOutFName = ioFolder + "/soilLoss_"+to_string(year[index])+"_" + to_string(scenario) + "_"+gid+".tif";
		outFName = tmpOutFName.c_str();
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData)
					{
					rowBuff[x] = (float)(eroMFD[x][y]-depoMFD[x][y]);
					}
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}		
		GDALClose( (GDALDatasetH) output );

		tmpOutFName = ioFolder + "/deposition_"+to_string(year[index])+"_" + to_string(scenario) + "_"+gid+".tif";
		outFName = tmpOutFName.c_str();
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData)
					{
					rowBuff[x] = (float)depoMFD[x][y];
					}
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}		
		GDALClose( (GDALDatasetH) output );

		tmpOutFName = ioFolder + "/gully_"+to_string(year[index])+"_"+gid+".tif";
		outFName = tmpOutFName.c_str();
		output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
		output->SetGeoTransform(adfGeoTransform);
		output->SetProjection(INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData)
					{
					rowBuff[x] = (bool)gully[x][y];
					}
				else
					rowBuff[x] = noData;
				}
			CPLErr err =  output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			if (err != CE_None) {fprintf(stderr, "RasterIO write failed at line %d (row %d)\n", __LINE__, y);}
			}
		GDALClose( (GDALDatasetH) output );
		}//end if erosion==1
		
