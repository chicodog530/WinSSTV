
void MainWindow::onPTTTestToggled(bool checked) {
  if (catEnabled && rigControlPtr) {
    if (checked) {
      rigControlPtr->setPTT(RIG_PTT_ON);
      pttTestButton->setText(tr("Test PTT (ON)"));
    } else {
      rigControlPtr->setPTT(RIG_PTT_OFF);
      pttTestButton->setText(tr("Test PTT (Toggle)"));
    }
  } else {
    // If CAT not enabled, just toggle the button text but do nothing real
    if (checked) {
      pttTestButton->setText(tr("Test PTT (ON - No CAT)"));
      pttTestButton->setChecked(false); // Reset immediately if no CAT
    }
  }
}
