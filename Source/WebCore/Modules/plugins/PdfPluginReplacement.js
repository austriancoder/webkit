
function createPluginReplacement(root, parent, attributeNames, attributeValues)
{
    return new Replacement(root, parent, attributeNames, attributeValues);
};

function Replacement(root, parent, attributeNames, attributeValues)
{
    this.root = root;
    this.parent = parent;
    this.height = "100%";
    this.width = "100%";
    this.src = "";

    this.createPdfElement(attributeNames, attributeValues);
};

Replacement.prototype = {

    AttributeMap: {
        height: 'height',
        width: 'width',
        src: 'src',
    },

    createPdfElement: function(attributeNames, attributeValues)
    {
        var pdf = this.pdf = document.createElement('pdf');

        console.log(attributeNames);

        for (i = 0; i < attributeNames.length; i++) {
            var property = this.AttributeMap[attributeNames[i]];

            if (this[property] != undefined)
                this[property] = attributeValues[i];
        }

        pdf.setAttribute('pseudo', '-webkit-plugin-replacement');

        // fixup be-http://.../*.pdf
        if (this.src.startsWith("be-"))
          this.src = this.src.substring(3);

        var u = "qrc:///pdf.js/web/viewer.html?file=" + encodeURI(this.src);
        u = "qrc:///pdf.js/web/viewer.html?file=" + this.src;

        //u = "qrc:///pdf.js/web/viewer.html";

        var ifrm = document.createElement("iframe");
        ifrm.setAttribute("scrolling", "no");
        ifrm.setAttribute("frameborder", "0");
        ifrm.setAttribute("src", u);
        ifrm.style.positiion = "abosulte";
        ifrm.style.height = this.height;
        ifrm.style.width = this.width;

        pdf.appendChild(ifrm);
        this.root.appendChild(pdf);
    },
};
