<script setup lang="ts">
import type {
  Scene,
  PerspectiveCamera,
  WebGLRenderer,
  Points,
  Material,
} from "three";

const container = ref<HTMLDivElement | null>(null);
const { isDark } = useTheme();

let THREE: typeof import("three");
let scene: Scene;
let camera: PerspectiveCamera;
let renderer: WebGLRenderer;
let particles: Points;
let animationId: number;

const particleCount = 1500;

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

    const color =
      colorOptions[Math.floor(Math.random() * colorOptions.length)]!;
    particleColors[i3] = color.r;
    particleColors[i3 + 1] = color.g;
    particleColors[i3 + 2] = color.b;

    sizes[i] = Math.random() * 3 + 1;
  }

  geometry.setAttribute("position", new THREE.BufferAttribute(positions, 3));
  geometry.setAttribute("color", new THREE.BufferAttribute(particleColors, 3));
  geometry.setAttribute("size", new THREE.BufferAttribute(sizes, 1));

  const material = new THREE.PointsMaterial({
    size: 0.12,
    vertexColors: true,
    transparent: true,
    opacity: 0.6,
    blending: THREE.NormalBlending,
    sizeAttenuation: true,
  });

  return new THREE.Points(geometry, material);
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

  const colorAttr = particles.geometry.attributes.color;
  if (!colorAttr) return;

  const particleColors = colorAttr.array as Float32Array;

  for (let i = 0; i < particleCount; i++) {
    const color =
      colorOptions[Math.floor(Math.random() * colorOptions.length)]!;
    const i3 = i * 3;
    particleColors[i3] = color.r;
    particleColors[i3 + 1] = color.g;
    particleColors[i3 + 2] = color.b;
  }

  colorAttr.needsUpdate = true;

  // Update background color for theme change
  if (scene) {
    scene.background = new THREE.Color(c.base);
  }
}

function animate() {
  animationId = requestAnimationFrame(animate);

  const time = Date.now() * 0.001;

  // Rotate particles slowly
  if (particles) {
    particles.rotation.y = time * 0.03;
    particles.rotation.x = Math.sin(time * 0.05) * 0.05;
  }

  renderer.render(scene, camera);
}

async function init() {
  if (!container.value) return;

  // Dynamic import of Three.js (client-side only)
  THREE = await import("three");

  const c = getColors();

  // Scene with Catppuccin background color
  scene = new THREE.Scene();
  scene.background = new THREE.Color(c.base);
  scene.fog = new THREE.FogExp2(c.base, 0.04);

  // Camera
  camera = new THREE.PerspectiveCamera(
    60,
    window.innerWidth / window.innerHeight,
    0.1,
    1000,
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
}

watch(isDark, () => {
  if (!THREE) return;
  updateParticleColors();
  if (scene) {
    const c = getColors();
    scene.fog = new THREE.FogExp2(c.base, 0.04);
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
  <div ref="container" class="fixed inset-0 z-0 blur-sm" aria-hidden="true" />
</template>
