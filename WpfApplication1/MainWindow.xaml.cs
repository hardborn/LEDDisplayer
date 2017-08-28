using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Newtonsoft.Json;

namespace WpfApplication1
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private System.Threading.Timer _timer;
        ScreenPlayer player;
        public MainWindow()
        {
            InitializeComponent();
            this.Loaded += MainWindow_Loaded;
        }

        void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            var action = new Action(() =>
            {
                StartPlay("");
            });
            action.BeginInvoke(null, null);

            //Thread thread = new Thread(StartPlay);
            //thread.IsBackground = true;
            //thread.Start();
        }
        private static OnCompletedPlayDelegate onCompletedPlay;
        // GCHandle gchCallbackDelegate;
        private void StartPlay(object obj)
        {
            player = new ScreenPlayer(400, 0, 200, 200);
            onCompletedPlay = new OnCompletedPlayDelegate(CompletedPlay);
            player.OnCompletedPlayHandler = onCompletedPlay;
            // gchCallbackDelegate = GCHandle.Alloc(onCompletedPlay);
            player.Initialize();

        }

        private void CompletedPlay(string filename)
        {
            var action = new Action(() =>
            {
                var obj = JsonConvert.DeserializeObject<Rootobject>(filename);
                System.Diagnostics.Debug.WriteLine(filename + "播放完成");

            });
            action.BeginInvoke(null, null);
        }


        private void Button_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "MP4|*.mp4;*.wmv;*.avi";
            openFileDialog.Multiselect = true;
            if (openFileDialog.ShowDialog() == true)
            {

                List<PlayInfoWrapper> playInfos = new List<PlayInfoWrapper>();
                foreach (var item in openFileDialog.FileNames)
                {
                    for (int i = 0; i < 1; i++)
                    {
                        playInfos.Add(new PlayInfoWrapper(item,string.Empty, 1, 100, 100, 1000, 800));
                    }

                }
                ScheduleInfoWrapper scheduleInfo = new ScheduleInfoWrapper(playInfos, 1);
                var action = new Action(() =>
                {
                    player.Play(scheduleInfo);
                });
                action.BeginInvoke(null, null);

            }

        }

        private void Button_Size_Click(object sender, RoutedEventArgs e)
        {
            int width = int.Parse(this.widthTextBox.Text);
            int height = int.Parse(this.heightTextBox.Text);
            player.UpdateScreenSize(0, 0, width, height);
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            player.Stop();
        }
    }


    public class Rootobject
    {
        public string MediaName { get; set; }
        [JsonConverter(typeof(MyDateTimeConverter))]
        public DateTime StartTime { get; set; }
        [JsonConverter(typeof(MyDateTimeConverter))]
        public DateTime EndTime { get; set; }
    }

    public class MyDateTimeConverter : Newtonsoft.Json.JsonConverter
    {
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(DateTime);
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            var t = (long)reader.Value;
            return new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc).AddSeconds(t);
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }
    }

}
