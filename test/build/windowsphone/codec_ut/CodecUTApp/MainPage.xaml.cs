using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;
using CodecUTApp.Resources;

using Codec_UT_RTComponent;

namespace CodecUTApp {
public partial class MainPage : PhoneApplicationPage {
  // Constructor
  bool bFlag;
  CodecUTTest cCodecUTHandler;
  public MainPage() {
    InitializeComponent();
    cCodecUTHandler = new CodecUTTest();
    // Sample code to localize the ApplicationBar
    //BuildLocalizedApplicationBar();
  }

  private void Button_Click (object sender, RoutedEventArgs e) {
    int iRetVal = 0;
    UTInfo.Text = "Running UT test cases! Please wait for 10~30 minutes ....";
    iRetVal = cCodecUTHandler.TestAllCases();
    if (0 == iRetVal) {
      UTInfo.Text = "Passed! UT cases on windows phone have been completed!";
    } else {
      UTInfo.Text = "Failed! UT cases on windows phone have been completed!";
    }
  }

  // Sample code for building a localized ApplicationBar
  //private void BuildLocalizedApplicationBar()
  //{
  //    // Set the page's ApplicationBar to a new instance of ApplicationBar.
  //    ApplicationBar = new ApplicationBar();

  //    // Create a new button and set the text value to the localized string from AppResources.
  //    ApplicationBarIconButton appBarButton = new ApplicationBarIconButton(new Uri("/Assets/AppBar/appbar.add.rest.png", UriKind.Relative));
  //    appBarButton.Text = AppResources.AppBarButtonText;
  //    ApplicationBar.Buttons.Add(appBarButton);

  //    // Create a new menu item with the localized string from AppResources.
  //    ApplicationBarMenuItem appBarMenuItem = new ApplicationBarMenuItem(AppResources.AppBarMenuItemText);
  //    ApplicationBar.MenuItems.Add(appBarMenuItem);
  //}
}
}