//Slope, aspect, curvature
for(int x=1;x<xSize-1;x++)
	{
	for(int y=1;y<ySize-1;y++)
		{	
		if(dem[x][y]!=noData&&dem[x-1][y]!=noData&&dem[x+1][y]!=noData&&dem[x][y-1]!=noData&&dem[x][y+1]!=noData)
			{
			float sx=((dem[x+1][y-1]+dem[x+1][y]+dem[x+1][y+1])-(dem[x-1][y-1]+dem[x-1][y]+dem[x-1][y+1]))/(6*dx);
			float sy=-((dem[x-1][y-1]+dem[x][y-1]+dem[x+1][y-1])-(dem[x-1][y+1]+dem[x][y+1]+dem[x+1][y+1]))/(6*dy);
			slope[x][y]=pow((pow(sx,2)+pow(sy,2)),.5);
			if(landuse[x][y]==30)//AR5:30
				dsc_max[x][y]=5;
			else if(landuse[x][y]>10&&landuse[x][y]<20)
				dsc_max[x][y]=1;				
			else if(landuse[x][y]>20&&landuse[x][y]<30)
				dsc_max[x][y]=3.27/(1 + exp(20*(slope[x][y]+.01)));
			}
		}
	}

cout<<"Calculating outflow ratios d[x,y,8]"<<endl;	
//flow distribution d

for(int x=1;x<xSize-1;x++)
	{
	for(int y=1;y<ySize-1;y++)
		{
		if(dem[x][y]!=noData&&lakes[x][y]==noData)
			{
			float sumDZ=0;
			int wIndex=0;
			for(int X=-1;X<2;X++)
				{
				for(int Y=-1;Y<2;Y++)
					{
					if(X==0&&Y==0)
						Y++;
					if(dem[x+X][y+Y]<dem[x][y]&&dem[x+X][y+Y]!=noData&&lakes[x+X][y+Y]==noData)
						sumDZ+=((dem[x][y]-dem[x+X][y+Y])/pow((pow(X,2)+pow(Y,2)),0.5));
					wIndex++;
					}
				}
		
			wIndex=0;
			for(int X=-1;X<2;X++)
				{
				for(int Y=-1;Y<2;Y++)
					{
					if(X==0&&Y==0)
						Y++;
					if(lakes[x+X][y+Y]!=noData)
						{
						for(int locX=-1;locX<2;locX++)
							{
							for(int locY=-1;locY<2;locY++)
								d[x+locX][y+locY][wIndex]=0;
							}
						d[x+X][y+Y][wIndex]=1;
						}
					if(dem[x+X][y+Y]<dem[x][y]&&dem[x+X][y+Y]!=noData&&lakes[x+X][y+Y]==noData)
						d[x][y][wIndex]=((dem[x][y]-dem[x+X][y+Y])/pow((pow(X,2)+pow(Y,2)),0.5))/sumDZ;
					else
						d[x][y][wIndex]=0;
					wIndex++;
					}
				}
			}
		}
	}
