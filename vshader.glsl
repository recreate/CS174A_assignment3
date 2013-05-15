#version 150 

in   vec4 vPosition;
in   vec3 vNormal;

// output values that will be interpretated per-fragment
out  vec3 fN;
out  vec3 fE;
out  vec3 fL;

out vec4 gouraudShading;

uniform mat4 ModelView;
uniform vec4 LightPosition;
uniform mat4 Projection;
uniform mat4 Transform;

// Used for Gouraud shading
uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;

uniform float shadingType;

void main()
{

	fN = (ModelView*Transform*vec4(vNormal,0)).xyz;
	fE = -(ModelView*Transform*vPosition).xyz;
	fL = (ModelView*(vec4(0.0,0.0,0.0,1.0)-Transform*vPosition+LightPosition)).xyz;

	if (shadingType == 1) {
		vec3 NN = normalize(fN);
		vec3 EE = normalize(fE);
		vec3 LL = normalize(fL);

		vec3 HH = normalize(LL + EE);

		float Kd = max(dot(LL, NN), 0.0);
		vec4 diffuse = Kd*DiffuseProduct;

		float Ks = pow(max(dot(NN, HH), 0.0), Shininess);
		vec4 specular = Ks*SpecularProduct;

		if( dot(LL, NN) < 0.0 ) {
			specular = vec4(0.0, 0.0, 0.0, 1.0);
		}

		gouraudShading = AmbientProduct + diffuse + specular;
		gouraudShading.a = 1.0;

	} else if (shadingType == 2) {
		
	}

	gl_Position = Projection*ModelView*Transform*vPosition;
}
