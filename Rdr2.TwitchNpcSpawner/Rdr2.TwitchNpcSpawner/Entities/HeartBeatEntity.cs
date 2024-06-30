namespace Rdr2.TwitchNpcSpawner.Entities;

using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

[Table("heartbeats"), Index(nameof(UserId))]
public class HeartBeatEntity
{
    [Key, DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int Id { get; init; }

    public required int UserId { get; init; }
    public required long LastBeatSec { get; set; }
}
