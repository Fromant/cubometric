#version 330 core
in vec3 vTexCoords;
out vec4 FragColor;

uniform float uTime; // Time in seconds
uniform vec3 uSunDirection; // Direction of the sun
uniform vec3 uMoonDirection; // Direction of the moon
uniform sampler2D sky;

// Function to compute gradient based on angle
vec3 getSkyColor(vec3 dir)
{
    float t = dot(dir, uSunDirection); // 1 = overhead, 0 = horizon, -1 = below
    float sunsetOffset = 0.55;

    vec3 nightColor = vec3(0.05, 0.1, 0.2);
    vec3 dayColor = vec3(0.2, 0.5, 1.0);
    vec3 sunsetColor = vec3(1.0, 0.5, 0.2);

    // Transition factors
    float sunsetFactor = smoothstep(0, 1, max(1 - abs(uSunDirection.y) / sunsetOffset, 0));
    //float nightFactor = smoothstep(0, 1, max(0, 1 - (uSunDirection.y + 1)/(1 - sunsetOffset)));

    // Interpolate between night and day
    vec3 skyColor = mix(nightColor, dayColor, clamp(uSunDirection.y / sunsetOffset + 1, 0, 1));

    // Overlay sunset orange when near horizon
    skyColor = mix(skyColor, sunsetColor, sunsetFactor);

    // Darken at night?
    // skyColor = mix(skyColor, nightColor, nightFactor);


    //skyColor = texture(sky, normalize(uSunDirection.xy+vTexCoords.xy)).rgb;
    //skyColor = mix(skyColor, vec3(texture(sky, vTexCoords.xy * 0.5 + 0.5).r), 0.5);

    return skyColor;
}

// Function to add sun glow
vec3 getSun(vec3 dir)
{
    float sunIntensity = max(dot(dir, normalize(uSunDirection)), 0.0);
    float glow = smoothstep(0.95, 1.0, sunIntensity);
    return vec3(1.0, 0.9, 0.6) * glow; // Sun color with glow
}

// Function to add moon glow
vec3 getMoon(vec3 dir)
{
    float moonIntensity = max(dot(dir, normalize(uMoonDirection)), 0.0);
    float glow = smoothstep(0.99, 0.999, moonIntensity);
    return vec3(0.8, 0.8, 1.0) * glow; // Moon color with glow
}

// Function for stars
vec3 getStars(vec3 dir)
{
    float starChance = fract(sin(dot(dir.xy ,vec2(12.9898,78.233))) * 43758.5453);
    float starBrightness = step(0.995 + uTime*0.0001 , starChance); // Random stars at night
    return vec3(starBrightness) * vec3(1.0); // White stars
}

void main()
{
    vec3 dir = normalize(vTexCoords); // Assuming vTexCoords is in world or local space

    vec3 color = getSkyColor(dir);

    // Add sun and moon depending on time/day cycle
    color += getSun(dir);
    color += getMoon(dir);

    // Add stars at night (when sun is below horizon)
//    if(dot(uSunDirection, dir) < -0.2)
//        color += getStars(dir);

    FragColor = vec4(color, 1.0);
}