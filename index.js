const htmlMinify = require('html-minifier');
const options = {
    minifyJS : true,
    minifyCSS: true,
    collapseWhitespace: true,
    maxLineLength: 50,
    removeComments: true,
    removeOptionalTags: true,
    removeRedundantAttributes: true,

}

const fs = require('fs');

fs.readFile('./index.html', 'utf8', (err, data) => {
  if (err) {
    console.error(err);
    return;
  }
  
  console.log(JSON.stringify(htmlMinify.minify(data, options)));
});