import { useEffect, useState } from 'react';

/**
 * A counter that changes whenever the operating system flips between light and
 * dark. Canvas pixels are painted once with whatever the CSS variables held at
 * the time, so anything drawn on a canvas has to repaint when this changes.
 */
export function useThemeEpoch(): number {
  const [epoch, setEpoch] = useState(0);
  useEffect(() => {
    const media = window.matchMedia('(prefers-color-scheme: dark)');
    const bump = () => setEpoch((e) => e + 1);
    media.addEventListener('change', bump);
    return () => media.removeEventListener('change', bump);
  }, []);
  return epoch;
}

export function prefersReducedMotion(): boolean {
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches;
}
