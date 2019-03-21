
/**
 * @fileoverview Custom functionality to apply throughout every adsize. This
 * has a dependency on common.js and utils.js
 */
var custom = (function() {

  /**
   * Classes which our JS hooks into. Add more class names as necessary.
   * @enum
   * @private
   */
  var elementClass_ = {
    item: 'js-item',
    itemBG: 'js-item-bg',
    itemName: 'js-item-name',
    itemPrice: 'js-item-price',
    itemSalePrice: 'js-item-saleprice',
    itemRegularPrice: 'js-item-regularprice',
    itemCTAOn: 'js-item-cta-on',
  };

  var elementId_ = {
    arrowPrevious: 'arrow-previous',
    arrowNext: 'arrow-next',
    gpaDataProvider: 'gpa-data-provider'
  };

  var ratingsArray = [];
  var regPricesArray = [];
  var salePricesArray = [];
  var theFrameOnAutoplay;

  /**
   * Initialization. Called from handleAdInitialized on each page.
   */
  function init() {
    utils.log('custom.init()');
    var data = common.getAdData();
    if (!data) return;

    // console.log("ratings array: " + ratingsArray);

    // If you're using the swipe gallery to display feed items.
    //initItemsUsingGallery_();
        //initItemsWithoutGallery_();

    // If you're NOT using the swipe gallery to display feed items.
    //initItemsWithoutGallery_();

    console.log("regular prices: ", regPricesArray);
    console.log("sale prices: ", salePricesArray);
    console.log("ratings: ", ratingsArray);

    // var logoImg = document.querySelector('#' + elementId_.logoImg);
    // logoImg.className += " horiz-center";
    // if ( !checkSizeLogoVertical() ) {
    //   console.log("logo should also move vertically");
    //   logoImg.className += " v-center";
    //   logoImg.style.setProperty ("position", "absolute", "important");
    // }
  }

  function galleryFrameShown(event) {
    var gallery = common.getGallery();
    var itemsLength = gallery.frameCount;
    var indexFirst = common.getGalleryFrameIndexFirst(event);
    var indexLast = common.getGalleryFrameIndexLast(event);
    var arrowPrevious = utils.getElement(elementId_.arrowPrevious);
    var arrowNext = utils.getElement(elementId_.arrowNext);

    if (itemsLength < 2) {
      utils.showElement(arrowPrevious, false);
      utils.showElement(arrowNext, false);
      return;
    }
    if (indexFirst > 0) {
      arrowPrevious.style.visibility = 'visible';
    } else {
      arrowPrevious.style.visibility = 'hidden';
    }
    if (indexLast < itemsLength - 1) {
      arrowNext.style.visibility = 'visible';

    } else {
      arrowNext.style.visibility = 'hidden';
    }
  }

  function transformDynamicData () {
    var dataProvider = document.querySelector('#' + elementId_.gpaDataProvider);
    console.log("data: " + dataProvider);
    dataProvider.addDataTransformer(function(dynamicData) {
        var itemAttributes = ['name', 'subTitle', 'price', 'description',];

        /**
        * Convert Japanese fullspace to ASCII halfspace.
        */
        if(dynamicData.Product){
          for (var i = 0, l = dynamicData.Product.length; i < l; i++) {
            convertFullspaceToHalfspace(dynamicData.Product[i], itemAttributes);
            priceFormat(dynamicData.Product[i]);
          }
        }
      var data = dynamicData;

      if (data.Product) {
        var rating = data.Product[0].advertiserReviewRating;
        var delimiter = 'advertiserReviewRating';

        if (rating == null) {
          rating = data.Product[0].advertiser_review_rating;
          delimiter = 'advertiser_review_rating';
          console.info("Rating System is using 'advertiser_review_rating'.")
        } else {
          console.info("Rating System is using 'advertiserReviewRating'.")
        }

        for (var i = 0; i < data.Product.length; i++) {
          /* ~~~~~~~~~~~~~
          THIS NEEDS TO BE TESTED IN ADWORDS
          ~~~~~~~~~~~~~~~ */
          if (delimiter == 'advertiser_review_rating') {
            ratingsArray.push(data.Product[i].advertiser_review_rating);
          } else {
            ratingsArray.push(data.Product[i].advertiserReviewRating);
          }

          /* ~~~~~~~~~~~~~
          DROP ALL CENTS VALUES
          ~~~~~~~~~~~~~~~ */
          if (data.Product[i].price) {
            data.Product[i].price = data.Product[i].price.replace(/(\.|\,)[\d]{2}(?![\d])/g,'');
            if (data.Product[i].salePrice) {
              data.Product[i].salePrice = data.Product[i].salePrice.replace(/(\.|\,)[\d]{2}(?![\d])/g,'');
            }
          }

          regPricesArray.push(data.Product[i].price);
          salePricesArray.push(data.Product[i].salePrice);

          data.Product[i].regularPrice = null;

          /* ~~~~~~~~~~~~~
          IF THERE IS A SALE PRICE AND THE SALE PRICE IS NOT EQUAL TO
          THE REGULAR PRICE
          ~~~~~~~~~~~~~~~ */
          if (data.Product[i].salePrice &&
              data.Product[i].salePrice != data.Product[i].price) {
            data.Product[i].regularPrice = data.Product[i].price;
            data.Product[i].price = data.Product[i].salePrice;

            /* ~~~~~~~~~~~~~
            CALCULATE THE % DISCOUNT
            ~~~~~~~~~~~~~~~ */
            data.Product[i].salePercentDiscount = null;
            data.Product[i].salePercentDiscount = calculateDiscount_(i);
          }
        }
      }
    });
  }

  function priceFormat (product) {
    if(!product.price) return;
    product.price = replaceChineseYenSign(product.price);
    product.price = removeCents(product.price);
  }

  /**
   * Replace double byte Chinese Yen sign (￥) to single byte Yen sign (¥).
   * @param {string} price Price attribute pottencially contains Yen sign.
   */
  function replaceChineseYenSign (price) {
    if(!price) return;

    if(price.indexOf("￥") != -1) {
      // Replacing Full Width Yen Sign (\uffe5) with Half Width Yen Sign (\u00a5).
      price = price.replace("\uffe5", "\u00a5");
    }

    return price;
  };

  /**
   * Remove cents where cents = '00'
   * @return {string} with cents removed
   */
  function removeCents (price) {
    if(! price) return;
    var matcher = /(\.|\,)0{2}(?![\d])/;
    return price.replace(matcher, '');
  };

  /**
   * Convert fullspace to ASCII halfspace.
   * @param {Object} item Item whose attributes are checked.
   * @param {Array} attrs Item attributes to check.
   */
  function convertFullspaceToHalfspace (item, attrs) {
    for (var i = 0; i < attrs.length; i++) {
      var attr = attrs[i];
      if(item[attrs[i]]) {
        item[attr] = convert(item[attr]);
      }
    }

    function convert (text) {
      if(text && text.indexOf("　") != -1) {
        text = text.replace(/　/g," ");
      }
      return text;
    }
  };

  function calculateDiscount_(index){
    var originalPrice = regPricesArray[index];
    var discountedPrice = salePricesArray[index];
    var discountRate;
    var matcher = /\D/g;

    if (originalPrice) {
      originalPrice = parseInt(originalPrice.replace(matcher, ''));
      discountedPrice = parseInt(discountedPrice.replace(matcher, ''));

      originalPrice = Math.abs(originalPrice);
      discountedPrice = Math.abs(discountedPrice);
      discountRate = (discountedPrice / originalPrice) * 100;
      discountRate = 100 - (discountRate);
      discountRate = Math.abs(Math.round(discountRate));
      discountRate = "-" + discountRate + "%";
    }

    return discountRate;
  }

  function checkSize(){
    var size = parseInt(common.getPageSize().width) + "x" + parseInt(common.getPageSize().height);
    var result = true;

    switch(size){
      case "320x500":
        result = false;
      break;
      default:
        result = true;
    }
    return result;
  }

  function checkSizeLogoVertical(){
    var size = parseInt(common.getPageSize().width) + "x" + parseInt(common.getPageSize().height);
    var result = true;

    switch(size){
      case "160x600": case "120x600": case "728x90": case "970x90": case "468x60":
        result = false;
      break;
      default:
        result = true;
    }
    return result;
  }



  /**
   * Find all items used in the swipe gallery and initialize custom behavior.
   * @private
   */
  function initItemsUsingGallery_() {
    var gallery = common.getGallery();

    // Apply settings to each item in the gallery
    var items = gallery.querySelectorAll('.' + elementClass_.item);
    theFrameOnAutoplay = items[common.getCurrentItemIndex()];
    for (var i = 0; i < items.length; i++) {
      var item = items[i];
      initItemDisplay_(item, i);
      setRatings_(item, i);
    }
  }

  /**
   * Find all items used outside the gallery and initialize custom behavior.
   * @private
   */
  function initItemsWithoutGallery_() {
    // Apply settings to each item
    var items = document.querySelectorAll('.' + elementClass_.item);
    for (var i = 0; i < items.length; i++) {
      var item = items[i];
      initItemDisplay_(item);
    }
  }

  function setRatings_(item, index) {
    var itemRating = item.querySelector('.' + elementClass_.itemRating);
    var bubbleHolders = itemRating.querySelectorAll('.' + elementClass_.bubbleHolder);
    var bubbleCount = parseFloat(ratingsArray[index]);

    if (bubbleCount > 5 || isNaN(bubbleCount)) return;
    if (bubbleCount % 1 != 0) {
      bubbleCount = Math.round(bubbleCount*2)/2;
    }
    // console.log("bubbleCount should be a number rounded to nearest .5: " + bubbleCount);

    for (var i = 0; i < bubbleHolders.length; i++) {
      bubbleHolders[i].querySelector('.' + elementClass_.bubbleEmpty).style.opacity = 1;
    }
    for (var j = 0; j < bubbleCount; j++) {
      bubbleHolders[j].querySelector('.' + elementClass_.bubbleEmpty).style.opacity = 0;
      bubbleHolders[j].querySelector('.' + elementClass_.bubbleFull).style.opacity = 1;
    }

    switch(bubbleCount){
      case 0.5: case 1.5: case 2.5: case 3.5: case 4.5:
        bubbleHolders[Math.floor(bubbleCount)].querySelector('.' + elementClass_.bubbleEmpty).style.opacity = 0;
        bubbleHolders[Math.floor(bubbleCount)].querySelector('.' + elementClass_.bubbleFull).style.opacity = 0;
        bubbleHolders[Math.floor(bubbleCount)].querySelector('.' + elementClass_.bubbleHalf).style.opacity = 1;
      break;
      default:
        // console.log("stars not on half");
    }
  }

  /**
   * Set each frame to active on swipegallery autoplay.
   * @param {Event} event Event fired once each frame autoplays.
   */
  function frameAutoplayed(event) {
    var gallery = event.target;
    var index = parseInt(event.detail.id) - 1;

    // Update the current item index.
    common.setCurrentItemIndex(index);

    // Apply mouseover / mouseout to relevant items.
    var items = gallery.querySelectorAll('.' + elementClass_.item);
    for (var i = 0; i < items.length; i++) {
      var item = items[i];
      if (common.isCurrentItemIndex(i)) {
        itemMouseOver(item);
      } else {
        itemMouseOut(item);
      }
    }

    theFrameOnAutoplay = items[common.getCurrentItemIndex()];

  }

  /**
   * On finalisation of swipegallery autoplay.
   * @param {Event} event Event fired once autoplay finished.
   */
  function autoplayEnded(event) {
    var gallery = event.target;
    // Set the current item index back to 0.
    common.setCurrentItemIndex(0);
  }

  /**
   * Set the display settings for each item.
   * Add any custom functionality you need applied on load.
   * @param {Element} item Item element.
   * @private
   */
  function initItemDisplay_(item, index) {

    // if you're using sales prices.
    // setSalePricesDisplay_(item);

    // Set mouseout.
    itemMouseOut(item);
  }

  /**
   * Sets the 3 price elements to display correctly when using sales price.
   * Find your price elements and set into common functionality.
   * @param {Element} item Item element.
   * @private
   */
  function setSalePricesDisplay_(item) {
    // Get reference to each price element.
    var itemPrice = item.querySelector('.' + elementClass_.itemPrice);
    var itemSalePrice = item.querySelector('.' + elementClass_.itemSalePrice);
    var itemRegularPrice = item.querySelector('.' + elementClass_.itemRegularPrice);

    // Sets each item to display correct prices.
    common.displayCorrectPrices(itemPrice, itemSalePrice, itemRegularPrice);

  }

  /**
   * Custom Item Mouse Interactions. Add your own behavior.
   */

  /**
   * Custom Mouseover interaction functionality.
   * @param {Element} item
   */
  function itemMouseOver(item) {
    if(!checkSize()) return;

    if (theFrameOnAutoplay) {
      itemMouseOut(theFrameOnAutoplay);
      // console.log("should stop running after the first mouseover has been done");
      theFrameOnAutoplay = undefined;
    }

    var itemBG = item.querySelector('.' + elementClass_.itemBG);
    var itemName = item.querySelector('.' + elementClass_.itemName);
    var itemCTAOn = item.querySelector('.' + elementClass_.itemCTAOn);

    itemBG.style.backgroundColor = "#f8f8fa";
    // itemBG.style.borderColor = "#fb0000";
    // itemName.style.fontWeight= "bold";
    itemCTAOn.style.opacity =  1;
  }

  /**
   * Custom Mouseout interaction functionality.
   * @param {Element} item
   */
  function itemMouseOut(item) {
    if(!checkSize()) return;
    var itemBG = item.querySelector('.' + elementClass_.itemBG);
    var itemName = item.querySelector('.' + elementClass_.itemName);
    var itemCTAOn = item.querySelector('.' + elementClass_.itemCTAOn);

    // itemBG.style.backgroundColor = "#ffffff";
    itemBG.style.borderColor = "#DBDBDB";
    // itemName.style.fontWeight= "normal";
    itemCTAOn.style.opacity =  0;
  }

  return {
    init: init,
    galleryFrameShown: galleryFrameShown,
    itemMouseOver: itemMouseOver,
    itemMouseOut: itemMouseOut,
    transformDynamicData: transformDynamicData,
    frameAutoplayed: frameAutoplayed,
    autoplayEnded: autoplayEnded
  };

})();
