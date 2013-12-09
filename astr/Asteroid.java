/* Author: Gabe Russotto - gabesotto@gmail.com 
 * Date  : 11/11/13
 * Class : Astr 3750 - Planets, Moons, and Rings
 * 
 * Astroids.java - This program simulates a test area of 500km being
 * 		struck by an asteroid every 1000 years until the test area reaches
 * 		saturation equalibrium. Each asteroid impact crater is 50km. 
 * 		When a crater's center is hit by a new asteroid's crater that 
 * 		crater is removed from the test area. 
 * 
 * Output - When saturation equalibrium is reached the program outputs
 * 		how many years it took for equalibrium to be reached. Also, 
 * 		every 100,000 years the number of asteroids in the test area is 
 * 		printed.
 */  

import java.awt.geom.*;
import java.util.*;
public class Asteroid{
	public static void main(String[] args){
		
		List<Ellipse2D> asteroids = new ArrayList<Ellipse2D>();
		List<Ellipse2D> asteroidsToRemove = new ArrayList<Ellipse2D>(); 
		Random randomGenerator = new Random();
		int years = 0;
		int x = randomGenerator.nextInt(500);
		int y = randomGenerator.nextInt(500);
		double density[] = new double[1000000];
		
		//initialize list with first asteroid
		asteroids.add(new Ellipse2D.Double(x, y, 50, 50));
	
		while(true){		
			years++;
						
			//generate circle of radius 50km on test area of 500km
			x = randomGenerator.nextInt(500); //generate random x value (0-499)
			y = randomGenerator.nextInt(500); //generate random y value (0-499)
			Ellipse2D newAsteroid = new Ellipse2D.Double(x, y, 50, 50); //Make circle of radius 50
			
			//find craters that were hit by new asteroids, if any.
			for(Ellipse2D asteroid : asteroids){
				if(newAsteroid.contains(asteroid.getX(), asteroid.getY())){ 
					asteroidsToRemove.add(asteroid);
				}
			}
			
			//remove all asteroids that were hit by new asteroid 
			asteroids.removeAll(asteroidsToRemove);
			
			//then add new asteroid to list
			asteroids.add(newAsteroid);
			
			//calculate density
			density[years] = (double)asteroids.size()/500.00;
			
			//calculate % change in density from time/2 year ago.
			double change = ((density[years] - density[years/2]) / density[years])*100;
			
			if(years % 100 == 0){
				System.out.println("Number of craters at " + (years * 1000) + " years: " + asteroids.size());
			}
			
			//if change in density from time/2 years ago is less then 5 percent saturation has been acheived.
			if(change <= 5){
				System.out.println("Saturation Equalibrium Acheived!");
				System.out.println("Time Taken: "  + years * 1000 + " years!");
				break;
			}
		}
	}	
}
