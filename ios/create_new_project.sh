#!/bin/bash

# Move to a temporary directory
cd /tmp

# Create a new Xcode project
xcodebuild -project "NewApp.xcodeproj" -list

# Copy the new project to replace the damaged one
cp -R NewApp.xcodeproj /Users/ymmtny/Documents/GitHub/Graphics-Extensions-for-Solar2D/ios/App.xcodeproj
