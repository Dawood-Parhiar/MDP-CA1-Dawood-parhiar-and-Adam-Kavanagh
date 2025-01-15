#version 120

//Shader to give water effects to the background 

 uniform float time; //for waves
 uniform vec2 resolution;
 uniform sampler2D texture;

 void main() {
     //normalized pixel coordinates
    vec2 uv = gl_FragCoord.xy / resolution;
    
     //calculate wave distortion using sine waves
     float wave = sin(uv.y * 30.0 + time) * 0.01; //horizontal waves
     wave += sin(uv.y * 60.0 + time * 1.5) * 0.02; //Add finer waves
     
     //Apply the wave distortion
     uv.x += wave;
     
     //sample the texture with the distorted UV coordinates
     vec4 color = texture2D(texture, uv);
    

  //output the final color
     gl_FragColor = color;
}
