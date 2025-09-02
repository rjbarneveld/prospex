if(scenario==0)//reduced tillage (spring tillage)
	{
	for(int y = 0; y < ySize; y++)
		{
		for(int x = 0; x < xSize; x++)
			{
			if(dem[x][y]!=noData&&texture[x][y]>=0&&channel[x][y]==noData&&lakes[x][y]==noData)
				{
				if(landuse[x][y]==21||landuse[x][y]==22||landuse[x][y]==23)
					{
					vegetation[x][y]=6;
//INCLUDE MANNING N
					}
				}
			}
		}
	}
	
else if(scenario==1)//reduced tillage (overvintring i stubb)
	{
	for(int y = 0; y < ySize; y++)
		{
		for(int x = 0; x < xSize; x++)
			{
			if(dem[x][y]!=noData&&texture[x][y]>=0&&channel[x][y]==noData&&lakes[x][y]==noData)
				{
				if(landuse[x][y]==21||landuse[x][y]==22||landuse[x][y]==23)
					{
					vegetation[x][y]=7;
					EHDepth[x][y][0]*=1;
					EHDepth[x][y][1]*=1;
					SWSat[x][y]*=1;
					SWSat_EHD[x][y]*=1;
					CC[x][y]=.8;//dummy
					drainageRate1[x][y]*=1;
					drainageRate2[x][y]*=1;
					erod[x][y]*=1;//erodibility
					FC[x][y]*=1;//field capacity
//INCLUDE MANNING N
					}
				}
			}
		}
	}

else if(scenario==2)//direct drilling
	{
	for(int y = 0; y < ySize; y++)
		{
		for(int x = 0; x < xSize; x++)
			{
			if(dem[x][y]!=noData&&texture[x][y]>=0&&channel[x][y]==noData&&lakes[x][y]==noData)
				{
				if(landuse[x][y]==21||landuse[x][y]==22||landuse[x][y]==23)
					{
					vegetation[x][y]=6;
					EHDepth[x][y][0]*=1.1;
					EHDepth[x][y][1]*=1.1;
					SWSat[x][y]*=1.1;
					SWSat_EHD[x][y]*=1.1;
					CC[x][y]=.85;//dummy
					drainageRate1[x][y]*=.9;
					drainageRate2[x][y]*=.9;
					erod[x][y]*=.51;//erodibility
					FC[x][y]*=1.2;//field capacity
//INCLUDE MANNING N
					}
				}
			}
		}
	}
	
else if(scenario==3)//strip till
	{
	for(int y = 0; y < ySize; y++)
		{
		for(int x = 0; x < xSize; x++)
			{
			if(dem[x][y]!=noData&&texture[x][y]>=0&&channel[x][y]==noData&&lakes[x][y]==noData)
				{
				int paramIndex=-1;
				if(landuse[x][y]==21||landuse[x][y]==22||landuse[x][y]==23)
					{
					vegetation[x][y]=6;
					EHDepth[x][y][0]*=1.1;
					EHDepth[x][y][1]*=1.1;
					SWSat[x][y]*=1.1;
					SWSat_EHD[x][y]*=1.1;
					CC[x][y]=.31;//dummy
					drainageRate1[x][y]*=.9;
					drainageRate2[x][y]*=.9;
					erod[x][y]*=.53;//erodibility
					FC[x][y]*=1.2;//field capacity
//INCLUDE MANNING N
					}
				}
			}
		}
	}
	
else if(scenario==4)//redusert jordarbeiding / harving
	{
	for(int y = 0; y < ySize; y++)
		{
		for(int x = 0; x < xSize; x++)
			{
			if(dem[x][y]!=noData&&texture[x][y]>=0&&channel[x][y]==noData&&lakes[x][y]==noData)
				{
				int paramIndex=-1;
				if(landuse[x][y]==21||landuse[x][y]==22||landuse[x][y]==23)
					{
					vegetation[x][y]=6;
					EHDepth[x][y][0]*=1.2;
					EHDepth[x][y][1]*=1.2;
					SWSat[x][y]*=1.1;
					SWSat_EHD[x][y]*=1.1;
					CC[x][y]=.26;//dummy
					drainageRate1[x][y]*=.9;
					drainageRate2[x][y]*=.9;
					erod[x][y]*=1;//erodibility
					FC[x][y]*=1.2;//field capacity
//INCLUDE MANNING N
					}
				}
			}
		}
	}
