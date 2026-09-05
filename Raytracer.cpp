
#include <iostream>
#include <fstream>
#include <string>
#include<sstream>
#include <vector>
#include <map>
#include <variant>
#include <type_traits>
#include <list>
#include <limits>
#include <tuple>
#include <cmath>
#include <algorithm>
#include <cstdio>

using namespace std;
float PI = acos(-1.0f);
// Define a simple Pixel structure to hold RGB values
struct Pixel {
	float r, g, b;
};
struct Mat {
	float Odr, Odg, Odb, Osr, Osg, Osb, K_a, K_d, K_s, n, opacity, refraction=-1.0f;
};
using TextureImage = vector<vector<Pixel>>;
struct Face {
	int v[3];   // vertex index
	int vt[3];  // texture index
	int vn[3];  // normal index
	int texID;
	int bumpID;
	Mat mtlcolor;
};
vector<Mat> MatList;
vector<vector<float>> SphereList, LightList, verticesList,vnList,vtList;
vector<Face> facesList;
vector<TextureImage> BumpList;
vector<TextureImage> TextureList;
//Save to the PPM File
void saveImage(const vector<vector<Pixel>>& image, int width, int height,string filename) {
	string Outputname = filename.substr(0, filename.size() - 4) + ".ppm";
	
	ofstream outfile(Outputname);
	outfile << "P3\n" << width << " " << height << "\n255\n";
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			outfile << image[j][i].r << " " << image[j][i].g << " " << image[j][i].b << " ";
		}
		outfile << "\n";
	}
	outfile.close();
	//cout << "Image saved to " << Outputname << endl;
}
using Dict = map<std::string, std::vector<float>>;
//Parameters to read form the input file
Dict dict = {
	{"imsize", {}},
	{"eyepos", {}},
	{"viewdir", {}},
	{"updir", {}},
	{"vfov", {}},
	{"bkgcolor", {}},
	{"mtlcolor", {}},
	{"sphere", {}},
	{"v", {}},
	{"f", {}},
	{"vn", {}},
	{"vt", {}},
	{"texture", {}},
	{"attlight", {}},
	{"bump", {}}

};
//Load the .PPM files
TextureImage LoadPPM(const string& textureFilename) {
	ifstream infile(textureFilename);
	if (!infile.is_open()) {
		cerr << "Error: Could not open texture file " << textureFilename << endl;
		return {};
	}
	string format;
	infile >> format;
	if (format != "P3") {
		cerr << "Error: Only P3 PPM format is supported. File: " << textureFilename << endl;
		return {};
	}
	int width, height, maxValue;
	infile >> width >> height >> maxValue;
	TextureImage texture(height, vector<Pixel>(width));
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			int r, g, b;
			if (!(infile >> r >> g >> b)) {
				cerr << "Error: Not enough data in texture file " << textureFilename << endl;
				return {};
			}
			// Convert value from [0, maxValue] to [0.0, 1.0]
			texture[j][i] = {
				static_cast<float>(r) / maxValue,
				static_cast<float>(g) / maxValue,
				static_cast<float>(b) / maxValue
			};
		}
	}
	return texture;
}
void AssignDict(string& filename, Dict& dict, vector<vector<float>>& SphereList, vector<vector<float>>& LightList, vector<Mat>& MatList)
{ 
	ifstream openfile(filename);
	if (!openfile.is_open()) {
		cerr << "Error: Could not open file " << filename << endl;
		return;
	}
	else {
		string line;
		while (getline(openfile, line)) {
			istringstream iss(line);
			string keyword;
			iss >> keyword;
			auto it = dict.find(keyword);
			if (it != dict.end()) {
				it->second.clear();
				if (keyword == "sphere") {
					vector<float> sphereinfo;
					float x;
					while (iss >> x) {
						sphereinfo.push_back(x);
					}
					it->second = sphereinfo; // Store the entire sphere information as a vector of floats
					//Sphere attached the newest texture color above it 
					float texID = TextureList.empty() ? -1 : (float)(TextureList.size() - 1);
					float bumpID = BumpList.empty() ? -1 : (float)(BumpList.size() - 1);
					sphereinfo.insert(sphereinfo.end(), texID);
					sphereinfo.insert(sphereinfo.end(), bumpID);
					SphereList.push_back(sphereinfo); // Add the sphere information to the SphereList

				
				}
				else if (keyword == "attlight") {
					vector<float> lightinfo;
					float x;
					while (iss >> x) {
						lightinfo.push_back(x);
						
					}
					it->second = lightinfo; 
				
					
					LightList.push_back(lightinfo); 
				}
				else if (keyword == "v") {
					vector<float> vertexinfo;
					float x;
					while (iss >> x) {
						vertexinfo.push_back(x);

					}
					it->second = vertexinfo; // Store the entire vertex information as a vector of floats
					verticesList.push_back(vertexinfo); // Add the vertex information to the vertices list
				}
				else if (keyword == "vn") {
					vector<float> vertexinfo;
					float x;
					while (iss >> x) {
						vertexinfo.push_back(x);

					}
					it->second = vertexinfo; // Store the normal vertices information
					vnList.push_back(vertexinfo); 
				}
				else if (keyword == "vt") {
					vector<float> vertexinfo;
					float x;
					while (iss >> x) {
						vertexinfo.push_back(x);
					}
					it->second = vertexinfo; // Store the texture vertices information
					vtList.push_back(vertexinfo);
				}
				else if (keyword == "f") {
					Face face;
					for (int i = 0; i < 3; i++) {
						string token;
						iss >> token;

						int v = 0, vt = 0, vn = 0;

						if (sscanf(token.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {
							face.v[i] = v;
							face.vt[i] = vt;
							face.vn[i] = vn;
						}
						else if (sscanf(token.c_str(), "%d/%d", &v, &vt) == 2) {
							face.v[i] = v;
							face.vt[i] = vt;
							face.vn[i] = 0;
						}
						else if (sscanf(token.c_str(), "%d//%d", &v, &vn) == 2) {
							face.v[i] = v;
							face.vt[i] = 0;
							face.vn[i] = vn;
						}
						else {
							face.v[i] = stoi(token);
							face.vt[i] = 0;
							face.vn[i] = 0;
						}
					}
					face.texID = TextureList.empty() ? -1 : (int)TextureList.size() - 1;
					face.bumpID = BumpList.empty() ? -1 : (int)BumpList.size() - 1;
					face.mtlcolor = MatList.empty() ? Mat() : MatList.back();
					facesList.push_back(face);
					
					
				}
				else if (keyword == "mtlcolor") {
					Mat m;
					float x;
					iss >> m.Odr >> m.Odg >> m.Odb >> m.Osr >> m.Osg >> m.Osb >> m.K_a >> m.K_d >> m.K_s >> m.n >> m.opacity >> m.refraction;
					MatList.push_back(m); // Add the material color information to the mtlcolor list
				}
				else if (keyword == "texture") {
					// Load the texture map 
					string textureFilename;
					iss >> textureFilename;
					// Load the texture image and store it in the TextureList, this part is not implemented in this code snippet
					TextureImage texture = LoadPPM(textureFilename);
					if (!texture.empty()) {
						TextureList.push_back(texture);
						cout << "Loaded texture: " << textureFilename
							<< " (" << texture[0].size() << "x" << texture.size() << ")" << endl;
					}


				}
				else if (keyword == "bump") {
				    //Load the bump map
					string bumpFilename;
					iss >> bumpFilename;
					// Load the bump map image and store it in the BumpList, this part is not implemented in this code snippet
					TextureImage bumpMap = LoadPPM(bumpFilename);
					if (!bumpMap.empty()) {
						BumpList.push_back(bumpMap);
						cout << "Loaded bump map: " << bumpFilename
							<< " (" << bumpMap[0].size() << "x" << bumpMap.size() << ")" << endl;
					}
					}
				else {
					float x;
					while (iss >> x) {
						it->second.push_back(x);
						cout << "Keyword: " << keyword << ", Value: " << x << endl;
					}
				}
				
			}
		


		}
		openfile.close();
		}
	}
		
	


void PrintDict(const std::map<std::string, std::vector<float>>& dict) {
	for (const auto& [key, vec] : dict) {
		std::printf("%s: ",key.c_str());
		for (size_t i = 0; i < vec.size(); ++i) {
			std::printf("%g%s", vec[i], (i + 1 < vec.size()) ? " " : "");
		}
		std::printf("\n");
	}
}
vector<float> ul, ur, lr, ll;
void CalculateCorners(const Dict& dict, vector<float>& ul, vector<float>& ur, vector<float>& lr, vector<float>& ll)
{
	if (dict.at("eyepos").size() != 3 || dict.at("viewdir").size() != 3 || dict.at("updir").size() != 3 || dict.at("vfov").size() != 1 || dict.at("imsize").size() != 2) {
		cerr << "Error: Invalid parameters for corner calculation." << endl;
		return;
	}
	// Extract parameters from the dictionary
	vector<float> eye = dict.at("eyepos");
	vector<float> view = dict.at("viewdir");
	vector<float> up = dict.at("updir");
	float vfov = dict.at("vfov")[0];
	int width = static_cast<int>(dict.at("imsize")[0]);
	int height = static_cast<int>(dict.at("imsize")[1]);
	// Calculate aspect ratio and other necessary vectors
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	// Further calculations would go here to determine the corners
	//U'=view x up
	vector<float> u = { view[1] * up[2] - view[2] * up[1], view[2] * up[0] - view[0] * up[2], view[0] * up[1] - view[1] * up[0] };
	vector<float>udir = { u[0] / sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]), u[1] / sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]), u[2] / sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]) };
	//V'=U' x view
	vector<float> v = { udir[1] * view[2] - udir[2] * view[1], udir[2] * view[0] - udir[0] * view[2], udir[0] * view[1] - udir[1] * view[0] };
	vector<float> vdir = { v[0] / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]), v[1] / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]), v[2] / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) };
	// Calculate the center of the image plane, arbritrarily set at a distance of 1 unit from the eye position along the view direction
	vector<float> center = { eye[0] + view[0], eye[1] + view[1], eye[2] + view[2] };
	// Calculate the height of the image plane based on the vertical field of view
	float imagePlaneHeight = 2.0f * tan(vfov * 0.5f * (3.14159265f / 180.0f));
	// Calculate the width of the image plane based on the aspect ratio
	float imagePlaneWidth = imagePlaneHeight * aspectRatio;
	// Calculate the corners of the image plane
	ul = { center[0] + (vdir[0] * (imagePlaneHeight / 2.0f)) - (udir[0] * (imagePlaneWidth / 2.0f)),
		   center[1] + (vdir[1] * (imagePlaneHeight / 2.0f)) - (udir[1] * (imagePlaneWidth / 2.0f)),
		   center[2] + (vdir[2] * (imagePlaneHeight / 2.0f)) - (udir[2] * (imagePlaneWidth / 2.0f)) };
	ur = { center[0] + (vdir[0] * (imagePlaneHeight / 2.0f)) + (udir[0] * (imagePlaneWidth / 2.0f)),
		center[1] + (vdir[1] * (imagePlaneHeight / 2.0f)) + (udir[1] * (imagePlaneWidth / 2.0f)),
		   center[2] + (vdir[2] * (imagePlaneHeight / 2.0f)) + (udir[2] * (imagePlaneWidth / 2.0f)) };
	ll = { center[0] - (vdir[0] * (imagePlaneHeight / 2.0f)) - (udir[0] * (imagePlaneWidth / 2.0f)),
		center[1] - (vdir[1] * (imagePlaneHeight / 2.0f)) - (udir[1] * (imagePlaneWidth / 2.0f)),
		center[2] - (vdir[2] * (imagePlaneHeight / 2.0f)) - (udir[2] * (imagePlaneWidth / 2.0f)) };
	lr = { center[0] - (vdir[0] * (imagePlaneHeight / 2.0f)) + (udir[0] * (imagePlaneWidth / 2.0f)),
		center[1] - (vdir[1] * (imagePlaneHeight / 2.0f)) + (udir[1] * (imagePlaneWidth / 2.0f)),
		center[2] - (vdir[2] * (imagePlaneHeight / 2.0f)) + (udir[2] * (imagePlaneWidth / 2.0f)) };

	
	// 
}
vector<vector<Pixel>> generateImage(int width, int height, const vector<vector<Pixel>>& matrix) {
	vector<vector<Pixel>> image(height, vector<Pixel>(width));
	// Function to generate an image based on the parsed dimensions  
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			// Set pixel color based on some criteria
			
			image[j][i] = matrix[j][i];
		}
	}
	return image;
}
tuple<bool, vector<float>> IntersectionInfo;

//Return float of the normal direction for each face, using the input face info
vector<float> LoadTextureS(float utex, float vtex, float texID)
{
	if (TextureList.empty() || TextureList[static_cast<int>(texID)].empty() || TextureList[static_cast<int>(texID)][0].empty()) {
		return { 1.0f, 1.0f, 1.0f };
	}
	int texHeight = static_cast<int>(TextureList[static_cast<int>(texID)].size());
	int texWidth = static_cast<int>(TextureList[static_cast<int>(texID)][0].size());
	int x = static_cast<int>(utex * (texWidth - 1));
	int y = static_cast<int>(vtex * (texHeight - 1));
	const Pixel& c = TextureList[static_cast<int>(texID)][y][x];
	return { c.r, c.g, c.b };
}
vector<float> LoadBumpS(float utex, float vtex, float texID)
{
	if (BumpList.empty() || BumpList[static_cast<int>(texID)].empty() || BumpList[static_cast<int>(texID)][0].empty()) {
		return { 1.0f, 1.0f, 1.0f };
	}
	int texHeight = static_cast<int>(BumpList[static_cast<int>(texID)].size());
	int texWidth = static_cast<int>(BumpList[static_cast<int>(texID)][0].size());
	int x = static_cast<int>(utex * (texWidth - 1));
	int y = static_cast<int>(vtex * (texHeight - 1));
	const Pixel& c = BumpList[static_cast<int>(texID)][y][x];
	return { c.r, c.g, c.b };
}
vector<float> LoadTexture(const vector<vector<float>>& vt, float beta, float gamma, int texid, int bump)
{

	if (vt.size() != 3 || vt[0].size() < 2 || vt[1].size() < 2 || vt[2].size() < 2) {
		return { 1.0f, 1.0f, 1.0f };
	}
	float alpha = 1.0f - beta - gamma;
	//Barycentric interpolation to calculate the exact UV coordinates
	float u = alpha * vt[0][0] + beta * vt[1][0] + gamma * vt[2][0];
	float v = alpha * vt[0][1] + beta * vt[1][1] + gamma * vt[2][1];
	int texHeight = static_cast<int>(TextureList[texid].size());
	int texWidth = static_cast<int>(TextureList[texid][0].size());
	//Clamp the UV textures
	u = max(0.0f, min(1.0f, u));
	v = max(0.0f, min(1.0f, v));
	//Project the UV coordinates to the texture image dimensions
	float x = u * (texWidth - 1);
	float y = v * (texHeight - 1);
	int x0 = static_cast<int>(floor(x));
	int y0 = static_cast<int>(floor(y));
	int x1 = min(x0 + 1, texWidth - 1);
	int y1 = min(y0 + 1, texHeight - 1);
	float fracX = x - x0;
	float fracY = y - y0;
	Pixel c00 = BumpList[texid][y0][x0];
	Pixel c10 = BumpList[texid][y0][x0];
	Pixel c01 = BumpList[texid][y0][x0];
	Pixel c11 = BumpList[texid][y0][x0];
	if (bump == 1)//int Bump is the parameter to determine whether to load the bump map or the texture map, if it is 1, load the bump map, otherwise load the texture map
	{
		c00 = BumpList[texid][y0][x0];
		c10 = BumpList[texid][y0][x1];
		c01 = BumpList[texid][y1][x0];
		c11 = BumpList[texid][y1][x1];
	}
	else
	{
		c00 = TextureList[texid][y0][x0];
		c10 = TextureList[texid][y0][x1];
		c01 = TextureList[texid][y1][x0];
		c11 = TextureList[texid][y1][x1];
	}
	//Bilear Interpolation using the four nearest texels
	float r = (1 - fracX) * (1 - fracY) * c00.r +
		fracX * (1 - fracY) * c10.r +
		(1 - fracX) * fracY * c01.r +
		fracX * fracY * c11.r;
	float g = (1 - fracX) * (1 - fracY) * c00.g +
		fracX * (1 - fracY) * c10.g +
		(1 - fracX) * fracY * c01.g +
		fracX * fracY * c11.g;

	float b = (1 - fracX) * (1 - fracY) * c00.b +
		fracX * (1 - fracY) * c10.b +
		(1 - fracX) * fracY * c01.b +
		fracX * fracY * c11.b;
	return { r, g, b };
}
tuple<bool, vector<float>> RenderFaces(const vector<float>& rayDirection, const vector<float>& eyeDirection, const vector<float>& faceinfo)
{
	vector<float> v1 = { verticesList[static_cast<int>(faceinfo[0]) - 1][0], verticesList[static_cast<int>(faceinfo[0]) - 1][1], verticesList[static_cast<int>(faceinfo[0]) - 1][2] };
	vector<float> v2 = { verticesList[static_cast<int>(faceinfo[1]) - 1][0], verticesList[static_cast<int>(faceinfo[1]) - 1][1], verticesList[static_cast<int>(faceinfo[1]) - 1][2] };
	vector<float> v3 = { verticesList[static_cast<int>(faceinfo[2]) - 1][0], verticesList[static_cast<int>(faceinfo[2]) - 1][1], verticesList[static_cast<int>(faceinfo[2]) - 1][2] };


	vector<float> E1 = { v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2] };
	vector<float> E2 = { v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2] };
	vector<float> N = { E1[1] * E2[2] - E1[2] * E2[1], E1[2] * E2[0] - E1[0] * E2[2], E1[0] * E2[1] - E1[1] * E2[0] };

	vector<float> P = { eyeDirection[0], eyeDirection[1], eyeDirection[2] };
	vector<float> R = { rayDirection[0], rayDirection[1], rayDirection[2] };
	float D = { -N[0] * v1[0] - N[1] * v1[1] - N[2] * v1[2] };
	//First solve for t, if it is larger than 0, and denomintor is not 0
	float denom = N[0] * R[0] + N[1] * R[1] + N[2] * R[2];
	if (denom == 0)
		return { false,vector<float>{} };
	float t = -(N[0] * P[0] + N[1] * P[1] + N[2] * P[2] + D) / denom;

	if (t <= 0)
		return { false,vector<float>{} };
	//Substiute t into equation of the equation to get the intersection point
	vector<float> P_intersect = { P[0] + t * R[0], P[1] + t * R[1], P[2] + t * R[2] };
	//Check if the intersection point is within the triangle using barycentric coordinates
	vector<float> C0 = { P_intersect[0] - v1[0], P_intersect[1] - v1[1], P_intersect[2] - v1[2] };
	//Check if the denominator for the matrix solution equals to zero
	float d11 = E1[0] * E1[0] + E1[1] * E1[1] + E1[2] * E1[2];
	float d22 = E2[0] * E2[0] + E2[1] * E2[1] + E2[2] * E2[2];
	float d12 = E1[0] * E2[0] + E1[1] * E2[1] + E1[2] * E2[2];
	float tri_denom = d11 * d22 - d12 * d12;
	if (tri_denom == 0)
	{

		printf("Error:Contains invalid faces");
		return { false, vector<float>{} };
	}
	else {

		float d1p = E1[0] * C0[0] + E1[1] * C0[1] + E1[2] * C0[2];
		float d2p = E2[0] * C0[0] + E2[1] * C0[1] + E2[2] * C0[2];
		float beta = (d22 * d1p - d12 * d2p) / (d11 * d22 - d12 * d12);
		float gamma = (d11 * d2p - d12 * d1p) / (d11 * d22 - d12 * d12);
		float alpha = 1 - beta - gamma;
		if (beta >= 0.0f && gamma >= 0.0f && beta + gamma <= 1.0f)
		{
			// inside triangle
			return { true, {t,alpha,beta,gamma} };
		}
		else
		{
			return  { false,vector<float>{} };
		}
	}
}

vector<float> CalculateNormal(const tuple<bool, vector<float>>& IntersectionInfo, const Face& face)
{
	//if it is not a smoothed out normal
	vector<float> faceinfo = { static_cast<float>(face.v[0]),  static_cast<float>(face.v[1]),  static_cast<float>(face.v[2]) }; // Extract the face information from the facesList
	vector<float> v1 = { verticesList[static_cast<int>(faceinfo[0]) - 1][0], verticesList[static_cast<int>(faceinfo[0]) - 1][1], verticesList[static_cast<int>(faceinfo[0]) - 1][2] };
	vector<float> v2 = { verticesList[static_cast<int>(faceinfo[1]) - 1][0], verticesList[static_cast<int>(faceinfo[1]) - 1][1], verticesList[static_cast<int>(faceinfo[1]) - 1][2] };
	vector<float> v3 = { verticesList[static_cast<int>(faceinfo[2]) - 1][0], verticesList[static_cast<int>(faceinfo[2]) - 1][1], verticesList[static_cast<int>(faceinfo[2]) - 1][2] };
	vector<float> E1 = { v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2] };
	vector<float> E2 = { v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2] };
	vector<float> N = { E1[1] * E2[2] - E1[2] * E2[1], E1[2] * E2[0] - E1[0] * E2[2], E1[0] * E2[1] - E1[1] * E2[0] };
	float alpha = get<1>(IntersectionInfo)[1];
	float beta = get<1>(IntersectionInfo)[2];
	float gamma = get<1>(IntersectionInfo)[3];
	
	
	if (face.bumpID != -1) {
		//If it has bump map, we will use the bump map to calculate the normal vector at the intersection point
		int bumpidx = 1;
		vector<vector<float>> vtface = { vtList[static_cast<int>(face.vt[0]) - 1], vtList[static_cast<int>(face.vt[1]) - 1], vtList[static_cast<int>(face.vt[2]) - 1] };
		vector<float> N_m = LoadTexture(vtface, beta, gamma, face.bumpID, bumpidx);

		N = {
			N_m[0] * 2.0f - 1.0f,
			N_m[1] * 2.0f - 1.0f,
			N_m[2] * 2.0f - 1.0f
		};
		float len = sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]);

		N = { N[0] / len, N[1] / len, N[2] / len };
	}
	else {
		if (face.vn[0] != 0) {
			//If it is a smooth normal, we will use the vertex normal to calculate the normal vector at the intersection point

			vector<float> n1 = { vnList[static_cast<int>(face.vn[0]) - 1][0], vnList[static_cast<int>(face.vn[0]) - 1][1], vnList[static_cast<int>(face.vn[0]) - 1][2] };
			vector<float> n2 = { vnList[static_cast<int>(face.vn[1]) - 1][0], vnList[static_cast<int>(face.vn[1]) - 1][1], vnList[static_cast<int>(face.vn[1]) - 1][2] };
			vector<float> n3 = { vnList[static_cast<int>(face.vn[2]) - 1][0], vnList[static_cast<int>(face.vn[2]) - 1][1], vnList[static_cast<int>(face.vn[2]) - 1][2] };
			N = { alpha * n1[0] + beta * n2[0] + gamma * n3[0], alpha * n1[1] + beta * n2[1] + gamma * n3[1], alpha * n1[2] + beta * n2[2] + gamma * n3[2] };
			N = { N[0] / sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]), N[1] / sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]), N[2] / sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]) }; // Normalize the normal vector
		}
		else {
			//If it is not a smooth normal, we will use the face normal to calculate the normal vector at the intersection point
			N = { E1[1] * E2[2] - E1[2] * E2[1], E1[2] * E2[0] - E1[0] * E2[2], E1[0] * E2[1] - E1[1] * E2[0] };
			N = { N[0] / sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]), N[1] / sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]), N[2] / sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]) }; // Normalize the normal vector
		}
	}
	return N;
}
//The recursive reflection function, input: the ray direction, the eye direction, and all the face information, including the face normal,

vector<float> RenderShadow(const vector<float>& P_intersect,
	const vector<float>& N,
	const vector<float>& light
	)
{
	bool isPointLight = (light[4] == 1.0f);
	const float EPSILON = 0.001f;
	float shadowcount = 0;	
	vector<float> shadowOrigin = {
		P_intersect[0] + N[0] * EPSILON,
		P_intersect[1] + N[1] * EPSILON,
		P_intersect[2] + N[2] * EPSILON
	};
	vector<float> shadowDir;
	float maxT = numeric_limits<float>::infinity();

	if (isPointLight) {
		shadowDir = {
			light[0] - shadowOrigin[0],
			light[1] - shadowOrigin[1],
			light[2] - shadowOrigin[2]
		};

		maxT = sqrt(
			shadowDir[0] * shadowDir[0] +
			shadowDir[1] * shadowDir[1] +
			shadowDir[2] * shadowDir[2]
		);

		shadowDir = {
			shadowDir[0] / maxT,
			shadowDir[1] / maxT,
			shadowDir[2] / maxT
		};
	}
	else {
		// Directional light
		shadowDir = light;
		float len = sqrt(
			shadowDir[0] * shadowDir[0] +
			shadowDir[1] * shadowDir[1] +
			shadowDir[2] * shadowDir[2]
		);
		shadowDir = {
			shadowDir[0] / len,
			shadowDir[1] / len,
			shadowDir[2] / len
		};
	}
	// Test against all faces
	for (const auto& face : facesList) {
		vector<float> faceinfo = {
			static_cast<float>(face.v[0]),
			static_cast<float>(face.v[1]),
			static_cast<float>(face.v[2])
		};
		tuple<bool, vector<float>> shadowHit =
			RenderFaces(shadowDir, shadowOrigin, faceinfo);
		if (get<0>(shadowHit)) {
			float tShadow = get<1>(shadowHit)[0];
			if (tShadow > EPSILON) {
				if (isPointLight) {
					if (tShadow < maxT) {
						shadowcount++;
						return { shadowcount,face.mtlcolor.opacity };
					}
				}
				else {
					shadowcount++;
					return{ shadowcount,face.mtlcolor.opacity };
				}
			}
		}
	}

	return { 0,1.0f }; // No shadow
}
Pixel CalculateColor(const vector<float>& rayDirection, const vector<float>& eyeDirection, int depth,float thetai,float opacity) {
	//Calculate the reflection direction based on the ray direction, the eye direction, and the face normal
	if (depth <= 0) {
		return { 0,0,0 };
	}
	float t = INFINITY;
	vector<float> N,Nr;
	vector<vector<float>> vtface;
	vector<float> O_s, O_d, R;
	float K_a, K_d, K_s, shininess, F0, newthetai, newalpha, air_refraction;
	bool isHit = false;
	bool hasrefraction;
	air_refraction = dict.at("bkgcolor")[3];
	for (const auto& face : facesList) {
		tuple<bool, vector<float>> IntersectionInfo;
		vector<float> faceinfo = { static_cast<float>(face.v[0]),  static_cast<float>(face.v[1]),  static_cast<float>(face.v[2]) };
		IntersectionInfo = RenderFaces(rayDirection, eyeDirection, faceinfo);
		if (get<0>(IntersectionInfo)) {
			isHit = true;
			float t_temp = get<1>(IntersectionInfo)[0];
		
			float alpha = get<1>(IntersectionInfo)[1];
			float beta = get<1>(IntersectionInfo)[2];
			float gamma = get<1>(IntersectionInfo)[3];

			

			if (t_temp < t) {
				t = t_temp;
				N = CalculateNormal(IntersectionInfo, face);
				//Find the intersection face normal direction N
				
				
				O_s = { face.mtlcolor.Osr,face.mtlcolor.Osg,face.mtlcolor.Osb };
				O_d = { face.mtlcolor.Odr,face.mtlcolor.Odg,face.mtlcolor.Odb };
				K_a = face.mtlcolor.K_a;
				K_d = face.mtlcolor.K_d;
				K_s = face.mtlcolor.K_s;
				F0 = ((face.mtlcolor.refraction - air_refraction) / (face.mtlcolor.refraction + air_refraction)) * ((face.mtlcolor.refraction - air_refraction) / (face.mtlcolor.refraction + air_refraction));
				shininess = face.mtlcolor.n;
				//refraction related
				float thetan = face.mtlcolor.refraction;
				
				float cosi_temp = N[0] * rayDirection[0] + N[1] * rayDirection[1] + N[2] * rayDirection[2];
				if (cosi_temp < 0) {

					cosi_temp = -cosi_temp;
					Nr = N;
				}
				else {
					//Inside the object and leaving it
					Nr = { -N[0], -N[1], -N[2] };
					thetan =air_refraction;

				}

				float cosi = -max(-1.0f, min(1.0f, Nr[0] * rayDirection[0] + Nr[1] * rayDirection[1] + Nr[2] * rayDirection[2]));
				float p = thetai / thetan;
				float k =1 - p * p * (1 - cosi * cosi);
				if (k >= 0) {
					hasrefraction = true;
				}
				else {
					hasrefraction = false;
				}
			    newthetai = thetan;
				newalpha = face.mtlcolor.opacity;
			    R = { rayDirection[0] * p + Nr[0] * (p * cosi - sqrt(k)), rayDirection[1] * p + Nr[1] * (p * cosi - sqrt(k)), rayDirection[2] * p + Nr[2] * (p * cosi - sqrt(k)) };
				float R_len = sqrt(R[0] * R[0] + R[1] * R[1] + R[2] * R[2]);
				R = { R[0] / R_len, R[1] / R_len, R[2] / R_len };

				if (face.texID != -1) {
					int bumpidx = 0;
					vtface = { vtList[static_cast<int>(face.vt[0]) - 1], vtList[static_cast<int>(face.vt[1]) - 1], vtList[static_cast<int>(face.vt[2]) - 1] };
					O_d = LoadTexture(vtface, beta, gamma, face.texID, bumpidx);
					O_s = { 0.44,0.59,0.66 };
					K_a = 0.3;
					K_d = 0.6;
					K_s = 0.9;
					shininess = 2;


				};
			}
		}
		else
			continue;

	}
	if (!isHit) {
		return { dict.at("bkgcolor")[0]/255.0f, dict.at("bkgcolor")[1]/255.0f, dict.at("bkgcolor")[2]/255.0f };
	}
	
	vector<float> P_intersect = { eyeDirection[0] + t * rayDirection[0], eyeDirection[1] + t * rayDirection[1], eyeDirection[2] + t * rayDirection[2] };
	vector<float> V = { eyeDirection[0] - P_intersect[0], eyeDirection[1] - P_intersect[1], eyeDirection[2] - P_intersect[2] };
	vector<float> V_norm = { V[0] / sqrt(V[0] * V[0] + V[1] * V[1] + V[2] * V[2]), V[1] / sqrt(V[0] * V[0] + V[1] * V[1] + V[2] * V[2]), V[2] / sqrt(V[0] * V[0] + V[1] * V[1] + V[2] * V[2]) };


	Pixel Color = { 0,0,0 };


	Color = { (O_d[0] * K_a), (O_d[1] * K_a), (O_d[2] * K_a) }; // Set the initial color based on ambient lightin

	vector <float> L; // Light direction, placeholder for actual light direction calculation
	float length;

	// Loop through each light source in the scene and calculate the contribution of each light to the final color of the pixel bein
	float w;
	for (const auto& light : LightList) {
		w = light[3];
		if (light[4] == 1.0f)//it is a point light
		{   
			
			L = { light[0] - P_intersect[0], light[1] - P_intersect[1], light[2] - P_intersect[2] };
			length = sqrt(L[0] * L[0] + L[1] * L[1] + L[2] * L[2]);
		}
		else if (light[4] == 0.0f)//it is a directional light
		{
			L = { light[0], light[1], light[2] };
			length = sqrt(L[0] * L[0] + L[1] * L[1] + L[2] * L[2]);
		}
		
		else {
			printf("Error: Invalid light type. Light type should be either 0 (directional) or 1 (point).");
			break;
		}
		vector<float>L_norm = { L[0] / sqrt(L[0] * L[0] + L[1] * L[1] + L[2] * L[2]), L[1] / sqrt(L[0] * L[0] + L[1] * L[1] + L[2] * L[2]), L[2] / sqrt(L[0] * L[0] + L[1] * L[1] + L[2] * L[2]) };

		vector<float>H_unnorm = { (L_norm[0] + V_norm[0]), (L_norm[1] + V_norm[1]) , (L_norm[2] + V_norm[2]) };
		vector<float>H = { H_unnorm[0] / (sqrt(H_unnorm[0] * H_unnorm[0] + H_unnorm[1] * H_unnorm[1] + H_unnorm[2] * H_unnorm[2])),H_unnorm[1] / (sqrt(H_unnorm[0] * H_unnorm[0] + H_unnorm[1] * H_unnorm[1] + H_unnorm[2] * H_unnorm[2])),H_unnorm[2] / (sqrt(H_unnorm[0] * H_unnorm[0] + H_unnorm[1] * H_unnorm[1] + H_unnorm[2] * H_unnorm[2])) };
		// Halfway vector between the light direction and the view direction
		//The light is related to  normal vector, a color vector O_d, in this case ignore the O_s and other material properties
		vector<float> shadowcount = RenderShadow(P_intersect, N, light);
		float shadowPercent = static_cast<float>(shadowcount[0]) / LightList.size(); // Ensure that the shadow count does not exceed the total number of light sources
		float shadowFactor = (1.0f - 0.8 * shadowPercent)*shadowcount[1]; // Calculate the shadow factor based on the percentage of light sources that are blocked
		Color = { Color.r + w *shadowFactor* O_d[0] * K_d * max(0.0f,(N[0] * L_norm[0] + N[1] * L_norm[1] + N[2] * L_norm[2])) + shadowFactor * w * K_s * O_s[0] * pow(max(0.0f, N[0] * H[0] + N[1] * H[1] + N[2] * H[2]),shininess),
					Color.g + w * shadowFactor*O_d[1] * K_d * max(0.0f,(N[0] *L_norm[0] + N[1] * L_norm[1] + N[2] * L_norm[2])) + shadowFactor *w * K_s * O_s[1] * pow(max(0.0f, N[0] * H[0] + N[1] * H[1] + N[2] * H[2]),shininess),
					Color.b + w * shadowFactor *O_d[2] * K_d * max(0.0f,(N[0] *L_norm[0] + N[1] * L_norm[1] + N[2] * L_norm[2])) + shadowFactor *w * K_s * O_s[2] * pow(max(0.0f, N[0] * H[0] + N[1] * H[1] + N[2] * H[2]),shininess) };
	}
	vector<float> reflectionDirection = {
	rayDirection[0] - 2.0f * (rayDirection[0] * N[0] + rayDirection[1] * N[1] + rayDirection[2] * N[2]) * N[0],
	rayDirection[1] - 2.0f * (rayDirection[0] * N[0] + rayDirection[1] * N[1] + rayDirection[2] * N[2]) * N[1],
	rayDirection[2] - 2.0f * (rayDirection[0] * N[0] + rayDirection[1] * N[1] + rayDirection[2] * N[2]) * N[2] };
	float len = sqrt(reflectionDirection[0] * reflectionDirection[0] + reflectionDirection[1] * reflectionDirection[1] + reflectionDirection[2] * reflectionDirection[2]);
	reflectionDirection = { reflectionDirection[0] / len, reflectionDirection[1] / len, reflectionDirection[2] / len };
	float Fr = F0 + (1.0f - F0) * pow(1.0f - max(0.0f, min(1.0f, -(rayDirection[0] * N[0] + rayDirection[1] * N[1] + rayDirection[2] * N[2]))), 5.0f);

	vector<float> refractionDirection = R;
	Pixel Rfterm;
	vector<float> refractionOrigin = { P_intersect[0] + refractionDirection[0] * 0.001f, P_intersect[1] + refractionDirection[1] * 0.001f, P_intersect[2] + refractionDirection[2] * 0.001f };
	if (hasrefraction)
	{
		Rfterm = CalculateColor(refractionDirection, refractionOrigin, depth - 1, newthetai, newalpha);
	}
	else
	{
		Rfterm = { 0,0,0 };
	}

	
		//Implement recursive reflection
	vector<float> P_new = { P_intersect[0] + reflectionDirection[0] * 0.001f, P_intersect[1] + reflectionDirection[1] * 0.001f, P_intersect[2] + reflectionDirection[2] * 0.001f };
	
	Pixel Rterm=CalculateColor(reflectionDirection, P_new,depth-1,newthetai,newalpha);
	
		
	return { Color.r + Fr * Rterm.r+(1-Fr)*(1-newalpha)*Rfterm.r,
		      Color.g + Fr * Rterm.g+(1-Fr)*(1- newalpha)*Rfterm.g,
			  Color.b + Fr * Rterm.b + (1 - Fr) * (1 - newalpha) * Rfterm.b
	};

}
			//reflected Direction
	
   
	
  
//Calculate the reflected direction based on the ray direction and the normal direction



vector<vector<Pixel>> Raycast(const Dict& dict,const vector<float>& ul, const vector<float>& ur, const vector<float>& lr, const vector<float>& ll)
{
	// This function would implement the raycasting logic to render the scene based on the corners and other parameters in the dictionary
	// Placeholder for raycasting implementation
	int width = static_cast<int>(dict.at("imsize")[0]);
	int height = static_cast<int>(dict.at("imsize")[1]);
	vector<float> eye = dict.at("eyepos");
//Temporary placeholder for sphere information, would need to be parsed properly based on the input format
	vector<vector<Pixel>> matrix(height, vector<Pixel>(width));
	vector<float> deltax = {
		(ur[0] - ul[0]) / (width - 1),
		(ur[1] - ul[1]) / (width - 1),
		(ur[2] - ul[2]) / (width - 1)
	};

	vector<float> deltay = {
		(ll[0] - ul[0]) / (height - 1),
		(ll[1] - ul[1]) / (height - 1),
		(ll[2] - ul[2]) / (height - 1)
	};

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			// Calculate the ray direction for the current pixel based on the corners and other parameters
			// Perform ray-sphere intersection tests and shading calculations
			matrix[j][i] = { 0,0,0 }; // Placeholder for actual color calculation based on ray-sphere intersection and shading
			///Ray intersect on the viewplane
			vector<float> rayIntersect = { ul[0] + i * deltax[0] + j * deltay[0], ul[1] + i * deltax[1] + j * deltay[1],ul[2] + i * deltax[2] + j * deltay[2] }; // Placeholder for actual ray direction calculation

			vector<float> rayDirction = { rayIntersect[0] - dict.at("eyepos")[0], rayIntersect[1] - dict.at("eyepos")[1], rayIntersect[2] - dict.at("eyepos")[2] }; // Placeholder for actual ray direction calculation
			float length = sqrt(rayDirction[0] * rayDirction[0] + rayDirction[1] * rayDirction[1] + rayDirction[2] * rayDirction[2]);
			rayDirction = { rayDirction[0] / length, rayDirction[1] / length, rayDirction[2] / length }; // Normalize the ray direction
			//Solve the Ray Equation in terms of t and check for intersection with spheres in the scene
			bool CheckIntersect = false;
			float closestT = numeric_limits<float>::max(); // Initialize closest intersection distance to a large value
			

					
		     matrix[j][i] = CalculateColor(rayDirction, dict.at("eyepos"), 3,1.0,0.0); // Placeholder for actual color calculation based on ray-sphere intersection and shading
		     matrix[j][i] = { min(255.0f, 255 * matrix[j][i].r), min(255.0f,  255 * matrix[j][i].g), min(255.0f,  255 * matrix[j][i].b) };
				}


			}

          
			
		
		//cout << "Finished processing row " << j + 1 << " of " << height << endl;

	//cout << "Finished processing pixel " << width << "x" << height << matrix[height - 1][width - 1].r << "," << matrix[height - 1][width - 1].g << "," << matrix[height - 1][width - 1].b << endl;
	auto Image = generateImage(width, height, matrix);
	//cout << "Raycasting completed. Image generated." << endl;
	cout << "Exporting image..." << endl;
	cout << Image.size() << endl;
	return Image;
	
}


int main(int argc, char* argv[]) {

	if (argc != 2) {
		
	}
	else {
		string filename = argv[1];


		AssignDict(filename, dict, SphereList, LightList, MatList);
		CalculateCorners(dict, ul, ur, lr, ll);
		auto exportImage = Raycast(dict, ul, ur, lr, ll);
		cout << "Exporting image to " << filename << endl;
		saveImage(exportImage, static_cast<int>(dict.at("imsize")[0]), static_cast<int>(dict.at("imsize")[1]), filename);

	}

}
