# Photon 1

Photon is our illumination engine. It handles both important things:

- Global Illumination that makes indirect lighting, making the scene feel more natural and realistic. (Using DDGI)
- Path Tracing that makes direct lighting, making the scene feel more dynamic and lively. (Using Metal Path Tracing)

## Spectral caustics

The Metal path tracer builds a separate caustic photon map for smooth specular surfaces. Photons start at directional, point, spot, rectangular area, and emissive triangle lights. They follow wavelength-dependent refraction, Fresnel reflection and total internal reflection, then deposit incident flux at the first non-delta receiver. Camera paths gather this flux through the receiver BRDF, including when viewing a receiver through glass. Straight shadow rays no longer apply the former artificial caustic gain. Area emitters are also visible to camera and specular rays.

The map traces 65,536 paths with at most 12 surface interactions and 32 intersection attempts per path. Its 16,384 hash buckets retain eight count-compensated photon samples each. Empty buckets are skipped; a gather examines at most 216 candidates. Photon records and buckets occupy approximately 4.06 MiB. Geometry, material and light changes invalidate the map; camera movement reuses it. Moving scenes still pay the bounded photon pass on each changed frame. Interactive camera rendering retains up to six bounces so entering and exiting glass does not immediately exhaust its path budget.

This is a fixed-radius density estimator, with the usual smoothing bias of [photon mapping](https://www.pbr-book.org/3ed-2018/Light_Transport_III_Bidirectional_Methods/Stochastic_Progressive_Photon_Mapping). The radius is 1.8% of the smooth geometry's bounding sphere radius, clamped to 0.01–0.15 world units. Spectral reconstruction uses a normalized Gaussian with a 10 nm standard deviation. Photon selection and hash collisions are count-compensated and filtered by cell, object, normal and distance; the cached map retains finite-sample noise. It is not progressive photon mapping. Rough transmission, nested refractive media and participating-media beams are outside this caustic pass; the existing path tracer continues to handle rough transport and environment illumination.

`AtlasDemos/Dispersion Demo` uses a white spotlight and a closed flint-glass prism to illuminate a neutral floor and screen. Its IOR and Abbe number approximate [SCHOTT N-SF10](https://media.schott.com/api/public/content/7c60bae06a384f1e93cf44f9f6f6ce4f?v=14b4674d) with the renderer's Cauchy dispersion model. The scene uses one camera sample per pixel, eight bounces, denoising and temporal accumulation. It contains no colored emissive beam geometry. Beams are not visible in clear air without a scattering medium.
