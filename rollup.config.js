export default {
  input: 'src/index.js',
  context: 'window',
  external: ['react', 'react-dom', 'decky-frontend-lib'],
  output: {
    file: 'dist/index.js',
    globals: {
      'react': 'SP_REACT',
      'react-dom': 'SP_REACTDOM',
      'decky-frontend-lib': 'DFL'
    },
    format: 'iife',
    exports: 'default',
  },
};
