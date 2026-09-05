# Recursive-raytracer-using-Blinn-Phong-shading
This project implements a c++ raytracer that render traingulated faces .obj models into Blinn Phong shaded model. Recursive tracing enables the simulation of reflecting and refraction based on user defined material index.
Input works for either UV mapped texture and user defined material parameter. 
Output is in P3 .ppm format.

The parameter for the material in the input txt:
Odr Odg Odb Osr Osg Osb ka kd ks n α η
(Od and Os represents the diffuse and specular parameter in the Blinn Phong model; ka, kd and ks denotes the ambient, diffusive and specular reflective coefficiant; n represents the shiness component, α means opacity and η is the index of refraction).

Example output:


<img width="656" height="660" alt="ppm sample" src="https://github.com/user-attachments/assets/a7a5f589-cef7-4c98-967f-b6aeffa99d03" />
