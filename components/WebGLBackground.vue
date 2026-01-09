<script setup lang="ts">
import type {
  Scene,
  PerspectiveCamera,
  WebGLRenderer,
  Points,
  BufferGeometry,
  Material,
} from "three";

const container = ref<HTMLDivElement | null>(null);
const { isDark } = useTheme();

let THREE: typeof import("three");
let scene: Scene;
let camera: PerspectiveCamera;
let renderer: WebGLRenderer;
let particles: Points;
let waveGeometry: BufferGeometry;
let wavePoints: Points;
let animationId: number;

const particleCount = 1500;
const waveParticleCount = 200;

// Catppuccin colors
const colors = {
  macchiato: {
    base: 0x24273a,
    surface0: 0x363a4f,
    mauve: 0xc6a0f6,
    lavender: 0xb7bdf8,
    sapphire: 0x7dc4e4,
    teal: 0x8bd5ca,
  },
  latte: {
    base: 0xeff1f5,
    surface0: 0xccd0da,
    mauve: 0x8839ef,
    lavender: 0x7287fd,
    sapphire: 0x209fb5,
    teal: 0x179299,
  },
};

function getColors() {
  return isDark.value ? colors.macchiato : colors.latte;
}

function createParticles() {
  const geometry = new THREE.BufferGeometry();
  const positions = new Float32Array(particleCount * 3);
  const particleColors = new Float32Array(particleCount * 3);
  const sizes = new Float32Array(particleCount);

  const c = getColors();
  const colorOptions = [
    new THREE.Color(c.mauve),
    new THREE.Color(c.lavender),
    new THREE.Color(c.sapphire),
    new THREE.Color(c.teal),
  ];

  for (let i = 0; i < particleCount; i++) {
    const i3 = i * 3;
    positions[i3] = (Math.random() - 0.5) * 20;
    positions[i3 + 1] = (Math.random() - 0.5) * 20;
    positions[i3 + 2] = (Math.random() - 0.5) * 10 - 5;

    const color = colorOptions[Math.floor(Math.random() * colorOptions.length)];
    particleColors[i3] = color.r;
    particleColors[i3 + 1] = color.g;
    particleColors[i3 + 2] = color.b;

    sizes[i] = Math.random() * 3 + 1;
  }

  geometry.setAttribute("position", new THREE.BufferAttribute(positions, 3));
  geometry.setAttribute("color", new THREE.BufferAttribute(particleColors, 3));
  geometry.setAttribute("size", new THREE.BufferAttribute(sizes, 1));

  const material = new THREE.PointsMaterial({
    size: 0.05,
    vertexColors: true,
    transparent: true,
    opacity: 0.6,
    blending: THREE.AdditiveBlending,
    sizeAttenuation: true,
  });

  return new THREE.Points(geometry, material);
}

function createWaveRing() {
  const geometry = new THREE.BufferGeometry();
  const positions = new Float32Array(waveParticleCount * 3);
  const particleColors = new Float32Array(waveParticleCount * 3);

  const c = getColors();
  const primaryColor = new THREE.Color(c.mauve);

  for (let i = 0; i < waveParticleCount; i++) {
    const angle = (i / waveParticleCount) * Math.PI * 2;
    const i3 = i * 3;
    positions[i3] = Math.cos(angle) * 2;
    positions[i3 + 1] = Math.sin(angle) * 2;
    positions[i3 + 2] = 0;

    particleColors[i3] = primaryColor.r;
    particleColors[i3 + 1] = primaryColor.g;
    particleColors[i3 + 2] = primaryColor.b;
  }

  geometry.setAttribute("position", new THREE.BufferAttribute(positions, 3));
  geometry.setAttribute("color", new THREE.BufferAttribute(particleColors, 3));

  const material = new THREE.PointsMaterial({
    size: 0.04,
    vertexColors: true,
    transparent: true,
    opacity: 0.8,
    blending: THREE.AdditiveBlending,
  });

  return new THREE.Points(geometry, material);
}

function updateWaveRing(time: number) {
  if (!waveGeometry) return;

  const positions = waveGeometry.attributes.position.array as Float32Array;

  for (let i = 0; i < waveParticleCount; i++) {
    const angle = (i / waveParticleCount) * Math.PI * 2;
    const waveOffset = Math.sin(angle * 8 + time * 2) * 0.15;
    const pulseOffset = Math.sin(time * 1.5) * 0.1;
    const radius = 2 + waveOffset + pulseOffset;

    const i3 = i * 3;
    positions[i3] = Math.cos(angle) * radius;
    positions[i3 + 1] = Math.sin(angle) * radius;
  }

  waveGeometry.attributes.position.needsUpdate = true;
}

function updateParticleColors() {
  if (!particles || !THREE) return;

  const c = getColors();
  const colorOptions = [
    new THREE.Color(c.mauve),
    new THREE.Color(c.lavender),
    new THREE.Color(c.sapphire),
    new THREE.Color(c.teal),
  ];

  const particleColors = particles.geometry.attributes.color.array as Float32Array;
  const waveColors = waveGeometry?.attributes.color.array as Float32Array;

  for (let i = 0; i < particleCount; i++) {
    const color = colorOptions[Math.floor(Math.random() * colorOptions.length)];
    const i3 = i * 3;
    particleColors[i3] = color.r;
    particleColors[i3 + 1] = color.g;
    particleColors[i3 + 2] = color.b;
  }

  if (waveColors) {
    const primaryColor = new THREE.Color(c.mauve);
    for (let i = 0; i < waveParticleCount; i++) {
      const i3 = i * 3;
      waveColors[i3] = primaryColor.r;
      waveColors[i3 + 1] = primaryColor.g;
      waveColors[i3 + 2] = primaryColor.b;
    }
    waveGeometry.attributes.color.needsUpdate = true;
  }

  particles.geometry.attributes.color.needsUpdate = true;

  // Update background color
  scene.background = new THREE.Color(c.base);
}

function animate() {
  animationId = requestAnimationFrame(animate);

  const time = Date.now() * 0.001;

  // Rotate particles slowly
  if (particles) {
    particles.rotation.y = time * 0.05;
    particles.rotation.x = Math.sin(time * 0.1) * 0.1;
  }

  // Animate wave ring
  updateWaveRing(time);

  // Subtle wave rotation
  if (wavePoints) {
    wavePoints.rotation.z = time * 0.2;
  }

  renderer.render(scene, camera);
}

async function init() {
  if (!container.value) return;

  // Dynamic import of Three.js (client-side only)
  THREE = await import("three");

  const c = getColors();

  // Scene
  scene = new THREE.Scene();
  scene.background = new THREE.Color(c.base);
  scene.fog = new THREE.Fog(c.base, 5, 15);

  // Camera
  camera = new THREE.PerspectiveCamera(
    60,
    window.innerWidth / window.innerHeight,
    0.1,
    1000
  );
  camera.position.z = 5;

  // Renderer
  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  container.value.appendChild(renderer.domElement);

  // Create particles
  particles = createParticles();
  scene.add(particles);

  // Create wave ring
  wavePoints = createWaveRing();
  waveGeometry = wavePoints.geometry;
  scene.add(wavePoints);

  // Handle resize
  window.addEventListener("resize", onResize);

  animate();
}

function onResize() {
  if (!camera || !renderer) return;

  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
}

function cleanup() {
  if (animationId) {
    cancelAnimationFrame(animationId);
  }
  window.removeEventListener("resize", onResize);

  if (renderer && container.value) {
    container.value.removeChild(renderer.domElement);
    renderer.dispose();
  }

  if (particles) {
    particles.geometry.dispose();
    (particles.material as Material).dispose();
  }

  if (wavePoints) {
    wavePoints.geometry.dispose();
    (wavePoints.material as Material).dispose();
  }
}

watch(isDark, () => {
  if (!THREE) return;
  updateParticleColors();
  if (scene) {
    const c = getColors();
    scene.fog = new THREE.Fog(c.base, 5, 15);
  }
});

onMounted(() => {
  init();
});

onUnmounted(() => {
  cleanup();
});
</script>

<template>
  <div
    ref="container"
    class="fixed inset-0 -z-10"
    aria-hidden="true"
  />
</template>
